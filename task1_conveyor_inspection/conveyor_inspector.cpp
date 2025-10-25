/**
 * 流水线产品质量检测系统 - 实现文件
 * 实现产品检测和追踪的核心功能
 */

#include "conveyor_inspector.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// ============================================================================
// ProductTracker 类实现
// ============================================================================

ProductTracker::ProductTracker(float dist_thresh)
    : next_id(0), distance_threshold(dist_thresh) {}

vector<TrackedProduct>& ProductTracker::update(const vector<Point2f>& centroids) {
    vector<TrackedProduct> new_tracked;

    for (const auto& centroid : centroids) {
        bool matched = false;

        for (auto& tracked : tracked_products) {
            float dist = norm(centroid - tracked.centroid);

            if (dist < distance_threshold) {
                tracked.centroid = centroid;
                tracked.frames_lost = 0;
                new_tracked.push_back(tracked);
                matched = true;
                break;
            }
        }

        if (!matched) {
            TrackedProduct new_product;
            new_product.id = next_id++;
            new_product.centroid = centroid;
            new_product.frames_lost = 0;
            new_product.counted = false;
            new_tracked.push_back(new_product);
        }
    }

    // 保留未匹配但丢失帧数较少的产品
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

// ============================================================================
// ConveyorInspector 类实现
// ============================================================================

ConveyorInspector::ConveyorInspector(bool save_frames)
    : save_frames(save_frames), frame_count(0),
      qualified_count(0), defective_count(0),
      counting_line_x(0) {}

vector<Detection> ConveyorInspector::detectProducts(const Mat& frame) {
    vector<Detection> detections;

    // 转换到HSV色彩空间
    Mat hsv, mask;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    // 检测绿色PCB
    Scalar lower_green(35, 40, 40);
    Scalar upper_green(85, 255, 255);
    inRange(hsv, lower_green, upper_green, mask);

    // 形态学操作
    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);
    morphologyEx(mask, mask, MORPH_OPEN, kernel);

    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area < 1000) continue;

        // 获取最小外接矩形
        RotatedRect rect = minAreaRect(contour);
        Point2f vertices[4];
        rect.points(vertices);

        // 计算轮廓逼近
        vector<Point> approx;
        approxPolyDP(contour, approx, arcLength(contour, true) * 0.04, true);

        Detection det;
        det.centroid = rect.center;
        det.rect = rect;
        det.box = vector<Point>(vertices, vertices + 4);
        det.angle = rect.angle;
        det.scale = max(rect.size.width, rect.size.height) / REFERENCE_SIZE;

        // 判断形状
        if (approx.size() == 3) {
            det.type = "defective";  // 三角形 = 次品
        } else {
            det.type = "qualified";  // 矩形 = 合格品
        }

        detections.push_back(det);
    }

    return detections;
}

void ConveyorInspector::updateCounts(const vector<Detection>& detections,
                                     vector<TrackedProduct>& tracked) {
    for (auto& track : tracked) {
        if (track.counted || track.centroid.x > counting_line_x) {
            continue;
        }

        // 找到对应的检测结果
        for (const auto& det : detections) {
            float dist = norm(det.centroid - track.centroid);
            if (dist < 50.0f) {
                track.counted = true;

                if (det.type == "qualified") {
                    qualified_count++;
                    cout << "Frame " << frame_count << ": QUALIFIED detected - "
                         << "Angle: " << fixed << setprecision(1) << det.angle << "°, "
                         << "Scale: " << setprecision(2) << det.scale << "x | "
                         << "Qualified: " << qualified_count
                         << ", Defective: " << defective_count << endl;
                } else {
                    defective_count++;
                    cout << "Frame " << frame_count << ": DEFECTIVE detected - "
                         << "Angle: " << fixed << setprecision(1) << det.angle << "°, "
                         << "Scale: " << setprecision(2) << det.scale << "x | "
                         << "Qualified: " << qualified_count
                         << ", Defective: " << defective_count << endl;
                }
                break;
            }
        }
    }
}

void ConveyorInspector::processVideo(const string& video_path) {
    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        cerr << "错误: 无法打开视频 " << video_path << endl;
        return;
    }

    int frame_width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    counting_line_x = frame_width * 0.2;  // 计数线位置

    cout << "============================================================" << endl;
    cout << "Processing: " << video_path << endl;
    cout << "============================================================" << endl;
    cout << endl;

    Mat frame;
    while (cap.read(frame)) {
        frame_count++;

        // 检测产品
        vector<Detection> detections = detectProducts(frame);

        // 提取质心用于追踪
        vector<Point2f> centroids;
        for (const auto& det : detections) {
            centroids.push_back(det.centroid);
        }

        // 更新追踪器
        vector<TrackedProduct>& tracked = tracker.update(centroids);

        // 更新计数
        updateCounts(detections, tracked);
    }

    cout << endl;
    cout << "视频处理完成！" << endl;
    cout << endl;
}

void ConveyorInspector::printStatistics(const string& video_path) {
    cout << "============================================================" << endl;
    cout << "最终统计报告" << endl;
    cout << "============================================================" << endl;
    cout << "视频: " << video_path << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "合格品数量: " << qualified_count << endl;
    cout << "次品数量:   " << defective_count << endl;
    cout << "总计:       " << (qualified_count + defective_count) << endl;
    cout << "============================================================" << endl;
    cout << endl;
}
