/**
 * Conveyor Belt Product Quality Inspection System (CLI Version)
 * Detects and classifies PCB boards as qualified (rectangle) or defective (triangle)
 * Command-line only version without GUI display
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>

using namespace cv;
using namespace std;

// Product tracking structure
struct TrackedProduct {
    int id;
    Point2f centroid;
    int frames_lost;
    bool counted;
};

// Detection result structure
struct Detection {
    string type;  // "qualified" or "defective"
    Point2f centroid;
    float angle;
    float scale;
    RotatedRect rect;
    vector<Point> box;
};

class ProductTracker {
private:
    vector<TrackedProduct> tracked_products;
    int next_id;
    float distance_threshold;

public:
    ProductTracker(float dist_thresh = 80.0f)
        : next_id(0), distance_threshold(dist_thresh) {}

    vector<TrackedProduct>& update(const vector<Point2f>& centroids) {
        vector<TrackedProduct> new_tracked;

        for (const auto& centroid : centroids) {
            bool matched = false;

            for (auto& tracked : tracked_products) {
                // Calculate distance
                float dist = norm(centroid - tracked.centroid);

                if (dist < distance_threshold) {
                    // Update existing track
                    tracked.centroid = centroid;
                    tracked.frames_lost = 0;
                    new_tracked.push_back(tracked);
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                // New product detected
                TrackedProduct new_product;
                new_product.id = next_id++;
                new_product.centroid = centroid;
                new_product.frames_lost = 0;
                new_product.counted = false;
                new_tracked.push_back(new_product);
            }
        }

        // Keep products that weren't matched for a few frames
        for (auto& tracked : tracked_products) {
            bool found = false;
            for (const auto& nt : new_tracked) {
                if (nt.id == tracked.id) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                tracked.frames_lost++;
                if (tracked.frames_lost < 10) {
                    new_tracked.push_back(tracked);
                }
            }
        }

        tracked_products = new_tracked;
        return tracked_products;
    }
};

class ConveyorInspector {
private:
    string video_path;
    VideoCapture cap;
    int qualified_count;
    int defective_count;
    ProductTracker tracker;
    int counting_line_x;
    bool save_debug_frames;
    string output_dir;

    Mat detectGreenObjects(const Mat& frame) {
        Mat hsv, mask;
        cvtColor(frame, hsv, COLOR_BGR2HSV);

        // Green color range for PCB boards
        Scalar lower_green(35, 40, 40);
        Scalar upper_green(85, 255, 255);

        inRange(hsv, lower_green, upper_green, mask);

        // Morphological operations
        Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
        morphologyEx(mask, mask, MORPH_CLOSE, kernel);
        morphologyEx(mask, mask, MORPH_OPEN, kernel);

        return mask;
    }

    string classifyProduct(const vector<Point>& contour, RotatedRect& rect, vector<Point>& box) {
        // Approximate polygon
        vector<Point> approx;
        double epsilon = 0.04 * arcLength(contour, true);
        approxPolyDP(contour, approx, epsilon, true);

        int num_vertices = approx.size();

        // Get rotated rectangle
        rect = minAreaRect(contour);
        Point2f vertices[4];
        rect.points(vertices);
        box.clear();
        for (int i = 0; i < 4; i++) {
            box.push_back(Point(vertices[i]));
        }

        // Calculate area and aspect ratio
        double area = contourArea(contour);
        float width = rect.size.width;
        float height = rect.size.height;

        if (width == 0 || height == 0 || area < 1000) {
            return "unknown";
        }

        float aspect_ratio = max(width, height) / min(width, height);

        // Triangle detection
        if (num_vertices <= 4 && area > 1000) {
            vector<Point> hull;
            convexHull(contour, hull);
            double hull_area = contourArea(hull);

            if (hull_area > 0) {
                float solidity = area / hull_area;

                // Triangles have lower solidity
                if (solidity < 0.7 || num_vertices == 3) {
                    return "defective";
                }
            }
        }

        // Rectangle detection
        if (num_vertices >= 4 && area > 1000) {
            return "qualified";
        }

        return "unknown";
    }

    pair<float, float> getRotationAndScale(const RotatedRect& rect, float reference_size = 200.0f) {
        float angle = rect.angle;
        float width = rect.size.width;
        float height = rect.size.height;

        // Normalize angle
        if (angle < -45) {
            angle = 90 + angle;
        }

        // Calculate scale
        float current_size = max(width, height);
        float scale_factor = current_size / reference_size;

        return make_pair(angle, scale_factor);
    }

public:
    ConveyorInspector(const string& video, bool save_frames = false)
        : video_path(video), qualified_count(0), defective_count(0),
          tracker(80.0f), counting_line_x(0), save_debug_frames(save_frames) {
        cap.open(video_path);

        if (save_frames) {
            // Create output directory based on video name
            size_t last_slash = video.find_last_of("/\\");
            size_t last_dot = video.find_last_of(".");
            string video_name = video.substr(last_slash + 1, last_dot - last_slash - 1);
            output_dir = "output_" + video_name;
            system(("mkdir -p " + output_dir).c_str());
        }
    }

    ~ConveyorInspector() {
        cap.release();
    }

    void processVideo() {
        if (!cap.isOpened()) {
            cerr << "错误：无法打开视频 " << video_path << endl;
            return;
        }

        int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
        int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
        int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
        double fps = cap.get(CAP_PROP_FPS);

        // Set counting line at 20% from left edge
        counting_line_x = frame_width * 0.2;

        // Track counted products
        set<int> counted_ids;

        cout << "\n" << string(60, '=') << endl;
        cout << "处理视频: " << video_path << endl;
        cout << "分辨率: " << frame_width << "x" << frame_height << endl;
        cout << "帧率: " << fps << " FPS" << endl;
        cout << "总帧数: " << total_frames << endl;
        cout << string(60, '=') << "\n" << endl;

        int frame_count = 0;
        int save_frame_interval = 30; // Save every 30 frames for debugging
        Mat frame, debug_frame, mask;

        while (true) {
            cap >> frame;
            if (frame.empty()) {
                break;
            }

            frame_count++;

            // Create debug frame for saving
            if (save_debug_frames) {
                debug_frame = frame.clone();
            }

            // Detect green objects
            mask = detectGreenObjects(frame);

            // Find contours
            vector<vector<Point>> contours;
            findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            // Process contours
            vector<Point2f> centroids;
            vector<Detection> detections;

            for (const auto& contour : contours) {
                double area = contourArea(contour);

                if (area < 1000) {
                    continue;
                }

                // Classify product
                Detection det;
                det.type = classifyProduct(contour, det.rect, det.box);

                if (det.type == "unknown") {
                    continue;
                }

                // Calculate centroid
                Moments m = moments(contour);
                if (m.m00 != 0) {
                    float cx = m.m10 / m.m00;
                    float cy = m.m01 / m.m00;
                    det.centroid = Point2f(cx, cy);
                    centroids.push_back(det.centroid);

                    // Get rotation and scale
                    auto rot_scale = getRotationAndScale(det.rect);
                    det.angle = rot_scale.first;
                    det.scale = rot_scale.second;

                    detections.push_back(det);
                }
            }

            // Update tracker
            auto& tracked = tracker.update(centroids);

            // Match detections with tracked products and count
            for (const auto& det : detections) {
                for (auto& track : tracked) {
                    float dist = norm(det.centroid - track.centroid);

                    if (dist < 50) {  // Match threshold
                        // Check if crossed counting line
                        if (det.centroid.x < counting_line_x &&
                            counted_ids.find(track.id) == counted_ids.end()) {

                            if (det.type == "qualified") {
                                qualified_count++;
                            } else {
                                defective_count++;
                            }

                            counted_ids.insert(track.id);

                            // Print real-time detection
                            cout << "帧 " << frame_count << ": "
                                 << (det.type == "qualified" ? "合格品" : "次品")
                                 << " 检测到 - 旋转角度: " << fixed << setprecision(1) << det.angle
                                 << "°, 缩放倍数: " << setprecision(2) << det.scale << "x | "
                                 << "合格品: " << qualified_count << ", "
                                 << "次品: " << defective_count << endl;
                        }
                        break;
                    }
                }

                // Draw detection on debug frame
                if (save_debug_frames) {
                    Scalar color = (det.type == "qualified") ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
                    polylines(debug_frame, det.box, true, color, 2);
                    circle(debug_frame, det.centroid, 5, Scalar(255, 0, 0), -1);

                    string label = (det.type == "qualified" ? "QUALIFIED" : "DEFECTIVE");
                    putText(debug_frame, label,
                           Point(det.centroid.x - 50, det.centroid.y - 10),
                           FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
                }
            }

            // Save debug frames periodically
            if (save_debug_frames && (frame_count % save_frame_interval == 0 || !detections.empty())) {
                // Draw counting line
                line(debug_frame, Point(counting_line_x, 0),
                     Point(counting_line_x, frame_height), Scalar(255, 255, 0), 2);

                // Draw statistics
                putText(debug_frame, "Qualified: " + to_string(qualified_count),
                       Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
                putText(debug_frame, "Defective: " + to_string(defective_count),
                       Point(10, 70), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);

                string filename = output_dir + "/frame_" +
                                 string(4 - to_string(frame_count).length(), '0') +
                                 to_string(frame_count) + ".jpg";
                imwrite(filename, debug_frame);
            }

            // Show progress
            if (frame_count % 30 == 0) {
                cout << "进度: " << frame_count << "/" << total_frames
                     << " (" << (frame_count * 100 / total_frames) << "%)" << endl;
            }
        }

        cout << "\n视频处理完成！" << endl;
        printReport();
    }

    void printReport() {
        cout << "\n" << string(60, '=') << endl;
        cout << "最终统计报告" << endl;
        cout << string(60, '=') << endl;
        cout << "视频: " << video_path << endl;
        cout << string(60, '-') << endl;
        cout << "合格品数量: " << qualified_count << endl;
        cout << "次品数量:   " << defective_count << endl;
        cout << "总计:       " << (qualified_count + defective_count) << endl;
        cout << string(60, '=') << "\n" << endl;
    }
};

int main(int argc, char** argv) {
    vector<string> video_files = {"../video/1.mp4", "../video/2.mp4"};

    bool save_frames = false;
    if (argc > 1 && string(argv[1]) == "--save-frames") {
        save_frames = true;
        cout << "调试模式：将保存检测帧到输出目录" << endl;
    }

    for (const auto& video_file : video_files) {
        ConveyorInspector inspector(video_file, save_frames);
        inspector.processVideo();
    }

    return 0;
}
