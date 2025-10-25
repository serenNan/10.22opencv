/**
 * 流水线产品质量检测系统 (命令行版本)
 * 检测并分类PCB板为合格品(矩形)或次品(三角形)
 * 纯命令行版本,无GUI显示
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>

using namespace cv;
using namespace std;

// 产品追踪结构体
struct TrackedProduct {
    int id;                // 产品唯一ID
    Point2f centroid;      // 质心坐标
    int frames_lost;       // 丢失帧数计数
    bool counted;          // 是否已统计
};

// 检测结果结构体
struct Detection {
    string type;           // "qualified"(合格) 或 "defective"(次品)
    Point2f centroid;      // 质心坐标
    float angle;           // 旋转角度
    float scale;           // 缩放倍数
    RotatedRect rect;      // 最小外接矩形
    vector<Point> box;     // 边界框顶点
};

// 产品追踪器类
class ProductTracker {
private:
    vector<TrackedProduct> tracked_products;  // 当前追踪的产品列表
    int next_id;                              // 下一个分配的产品ID
    float distance_threshold;                 // 距离匹配阈值

public:
    // 构造函数,初始化距离阈值(默认80像素)
    ProductTracker(float dist_thresh = 80.0f)
        : next_id(0), distance_threshold(dist_thresh) {}

    // 更新追踪器:将当前帧检测到的质心与已追踪的产品进行匹配
    vector<TrackedProduct>& update(const vector<Point2f>& centroids) {
        vector<TrackedProduct> new_tracked;

        for (const auto& centroid : centroids) {
            bool matched = false;

            for (auto& tracked : tracked_products) {
                // 计算距离
                float dist = norm(centroid - tracked.centroid);

                if (dist < distance_threshold) {
                    // 更新现有追踪目标
                    tracked.centroid = centroid;
                    tracked.frames_lost = 0;
                    new_tracked.push_back(tracked);
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                // 检测到新产品
                TrackedProduct new_product;
                new_product.id = next_id++;
                new_product.centroid = centroid;
                new_product.frames_lost = 0;
                new_product.counted = false;
                new_tracked.push_back(new_product);
            }
        }

        // 保留未匹配但丢失帧数较少的产品(允许短暂遮挡)
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
                if (tracked.frames_lost < 10) {  // 保留10帧内丢失的目标
                    new_tracked.push_back(tracked);
                }
            }
        }

        tracked_products = new_tracked;
        return tracked_products;
    }
};

// 流水线检测器主类
class ConveyorInspector {
private:
    string video_path;           // 视频文件路径
    VideoCapture cap;            // 视频捕获对象
    int qualified_count;         // 合格品计数
    int defective_count;         // 次品计数
    ProductTracker tracker;      // 产品追踪器
    int counting_line_x;         // 计数线X坐标
    bool save_debug_frames;      // 是否保存调试帧
    string output_dir;           // 输出目录

    // 检测绿色PCB对象
    Mat detectGreenObjects(const Mat& frame) {
        Mat hsv, mask;
        cvtColor(frame, hsv, COLOR_BGR2HSV);

        // PCB板的绿色范围(HSV色彩空间)
        Scalar lower_green(35, 40, 40);
        Scalar upper_green(85, 255, 255);

        inRange(hsv, lower_green, upper_green, mask);

        // 形态学操作:闭运算填充孔洞,开运算去除噪声
        Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
        morphologyEx(mask, mask, MORPH_CLOSE, kernel);
        morphologyEx(mask, mask, MORPH_OPEN, kernel);

        return mask;
    }

    // 产品分类函数:根据轮廓形状判断合格品(矩形)或次品(三角形)
    string classifyProduct(const vector<Point>& contour, RotatedRect& rect, vector<Point>& box) {
        // 多边形近似:简化轮廓,减少顶点数
        vector<Point> approx;
        double epsilon = 0.04 * arcLength(contour, true);
        approxPolyDP(contour, approx, epsilon, true);

        int num_vertices = approx.size();  // 近似多边形的顶点数

        // 获取最小外接旋转矩形
        rect = minAreaRect(contour);
        Point2f vertices[4];
        rect.points(vertices);
        box.clear();
        for (int i = 0; i < 4; i++) {
            box.push_back(Point(vertices[i]));
        }

        // 计算面积和长宽比
        double area = contourArea(contour);
        float width = rect.size.width;
        float height = rect.size.height;

        if (width == 0 || height == 0 || area < 1000) {
            return "unknown";  // 面积过小或无效
        }

        float aspect_ratio = max(width, height) / min(width, height);

        // 三角形检测(次品)
        if (num_vertices <= 4 && area > 1000) {
            vector<Point> hull;
            convexHull(contour, hull);
            double hull_area = contourArea(hull);

            if (hull_area > 0) {
                float solidity = area / hull_area;  // 实心度:轮廓面积/凸包面积

                // 三角形的实心度较低
                if (solidity < 0.7 || num_vertices == 3) {
                    return "defective";  // 次品
                }
            }
        }

        // 矩形检测(合格品)
        if (num_vertices >= 4 && area > 1000) {
            return "qualified";  // 合格品
        }

        return "unknown";
    }

    // 获取旋转角度和缩放倍数
    pair<float, float> getRotationAndScale(const RotatedRect& rect, float reference_size = 200.0f) {
        float angle = rect.angle;
        float width = rect.size.width;
        float height = rect.size.height;

        // 归一化角度到[-45, 45]范围
        if (angle < -45) {
            angle = 90 + angle;
        }

        // 计算缩放倍数(相对于参考尺寸200像素)
        float current_size = max(width, height);
        float scale_factor = current_size / reference_size;

        return make_pair(angle, scale_factor);
    }

public:
    // 构造函数:初始化检测器
    ConveyorInspector(const string& video, bool save_frames = false)
        : video_path(video), qualified_count(0), defective_count(0),
          tracker(80.0f), counting_line_x(0), save_debug_frames(save_frames) {
        cap.open(video_path);

        if (save_frames) {
            // 根据视频名称创建输出目录
            size_t last_slash = video.find_last_of("/\\");
            size_t last_dot = video.find_last_of(".");
            string video_name = video.substr(last_slash + 1, last_dot - last_slash - 1);
            output_dir = "output_" + video_name;
            system(("mkdir -p " + output_dir).c_str());
        }
    }

    // 析构函数:释放视频捕获资源
    ~ConveyorInspector() {
        cap.release();
    }

    // 主视频处理函数
    void processVideo() {
        if (!cap.isOpened()) {
            cerr << "错误：无法打开视频 " << video_path << endl;
            return;
        }

        int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
        int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
        int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
        double fps = cap.get(CAP_PROP_FPS);

        // 设置计数线位置:距离左边缘20%处
        counting_line_x = frame_width * 0.2;

        // 记录已统计的产品ID,避免重复计数
        set<int> counted_ids;

        cout << "\n" << string(60, '=') << endl;
        cout << "处理视频: " << video_path << endl;
        cout << "分辨率: " << frame_width << "x" << frame_height << endl;
        cout << "帧率: " << fps << " FPS" << endl;
        cout << "总帧数: " << total_frames << endl;
        cout << string(60, '=') << "\n" << endl;

        int frame_count = 0;
        int save_frame_interval = 30;  // 每30帧保存一次调试帧
        Mat frame, debug_frame, mask;

        while (true) {
            cap >> frame;
            if (frame.empty()) {
                break;  // 视频结束
            }

            frame_count++;

            // 创建调试帧副本用于保存
            if (save_debug_frames) {
                debug_frame = frame.clone();
            }

            // 检测绿色PCB对象
            mask = detectGreenObjects(frame);

            // 查找轮廓
            vector<vector<Point>> contours;
            findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            // 处理轮廓
            vector<Point2f> centroids;  // 质心列表
            vector<Detection> detections;  // 检测结果列表

            for (const auto& contour : contours) {
                double area = contourArea(contour);

                if (area < 1000) {
                    continue;  // 过滤面积过小的轮廓(噪声)
                }

                // 产品分类
                Detection det;
                det.type = classifyProduct(contour, det.rect, det.box);

                if (det.type == "unknown") {
                    continue;  // 跳过无法识别的对象
                }

                // 计算质心
                Moments m = moments(contour);
                if (m.m00 != 0) {
                    float cx = m.m10 / m.m00;
                    float cy = m.m01 / m.m00;
                    det.centroid = Point2f(cx, cy);
                    centroids.push_back(det.centroid);

                    // 获取旋转角度和缩放倍数
                    auto rot_scale = getRotationAndScale(det.rect);
                    det.angle = rot_scale.first;
                    det.scale = rot_scale.second;

                    detections.push_back(det);
                }
            }

            // 更新追踪器
            auto& tracked = tracker.update(centroids);

            // 将检测结果与追踪的产品进行匹配并计数
            for (const auto& det : detections) {
                for (auto& track : tracked) {
                    float dist = norm(det.centroid - track.centroid);

                    if (dist < 50) {  // 匹配阈值
                        // 检查是否越过计数线
                        if (det.centroid.x < counting_line_x &&
                            counted_ids.find(track.id) == counted_ids.end()) {

                            if (det.type == "qualified") {
                                qualified_count++;
                            } else {
                                defective_count++;
                            }

                            counted_ids.insert(track.id);

                            // 实时输出检测结果
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

                // 在调试帧上绘制检测结果
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

            // 定期保存调试帧
            if (save_debug_frames && (frame_count % save_frame_interval == 0 || !detections.empty())) {
                // 绘制计数线
                line(debug_frame, Point(counting_line_x, 0),
                     Point(counting_line_x, frame_height), Scalar(255, 255, 0), 2);

                // 绘制统计信息
                putText(debug_frame, "Qualified: " + to_string(qualified_count),
                       Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
                putText(debug_frame, "Defective: " + to_string(defective_count),
                       Point(10, 70), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);

                string filename = output_dir + "/frame_" +
                                 string(4 - to_string(frame_count).length(), '0') +
                                 to_string(frame_count) + ".jpg";
                imwrite(filename, debug_frame);
            }

            // 显示处理进度
            if (frame_count % 30 == 0) {
                cout << "进度: " << frame_count << "/" << total_frames
                     << " (" << (frame_count * 100 / total_frames) << "%)" << endl;
            }
        }

        cout << "\n视频处理完成！" << endl;
        printReport();
    }

    // 打印最终统计报告
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

// 主函数
int main(int argc, char** argv) {
    // 检查命令行参数
    if (argc < 2) {
        cerr << "用法: " << argv[0] << " <视频路径>" << endl;
        cerr << "示例: " << argv[0] << " ../video/1.mp4" << endl;
        return 1;
    }

    // 获取视频路径
    string video_path = argv[1];

    // 创建检测器并处理视频
    ConveyorInspector inspector(video_path, false);
    inspector.processVideo();

    return 0;
}
