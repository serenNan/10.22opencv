/**
 * 流水线产品质量检测系统 - 实现文件
 * 实现产品检测和追踪的核心功能
 */

#include "conveyor_inspector.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
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
                tracked.frames_tracked++;
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
            new_product.initial_pos = centroid;  // 记录初始位置
            new_product.frames_tracked = 1;
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

ConveyorInspector::ConveyorInspector()
    : frame_count(0), qualified_count(0), defective_count(0),
      reference_size(0.0f), reference_initialized(false) {}

// 计算矩形旋转角度(正置=长边水平为0°)
float ConveyorInspector::calculateRectangleAngle(const RotatedRect& rect) {
    float angle = rect.angle;
    float width = rect.size.width;
    float height = rect.size.height;

    if (width < height) {
        swap(width, height);
        angle += 90.0f;
    }

    while (angle < 0) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;

    return angle;
}

vector<Detection> ConveyorInspector::detectProducts(const Mat& frame) {
    vector<Detection> detections;

    Mat hsv, mask;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    Scalar lower_white(0, 0, 200);
    Scalar upper_white(179, 30, 255);
    inRange(hsv, lower_white, upper_white, mask);

    bitwise_not(mask, mask);

    // 形态学操作:开运算去噪 + 闭运算填充空洞
    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(mask, mask, MORPH_OPEN, kernel, Point(-1,-1), 2);
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);

    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area < 5000) continue;  // 过滤小噪声

        // 获取最小外接矩形
        RotatedRect rect = minAreaRect(contour);
        Point2f vertices[4];
        rect.points(vertices);

        vector<Point> approx;
        approxPolyDP(contour, approx, arcLength(contour, true) * 0.03, true);

        float width = rect.size.width;
        float height = rect.size.height;
        float area_ratio = area / (width * height);

        // 矩形判定:4顶点 + 填充度>0.80
        bool is_rectangular = (approx.size() == 4 && area_ratio > 0.80);

        Detection det;
        det.centroid = rect.center;
        det.rect = rect;
        det.box = vector<Point>(vertices, vertices + 4);

        if (is_rectangular) {
            det.type = "qualified";
            det.angle = calculateRectangleAngle(rect);

            // 初始化缩放基准（使用首个合格品的长边尺寸）
            float current_size = max(width, height);
            if (!reference_initialized) {
                reference_size = current_size;
                reference_initialized = true;
                cout << "  [缩放基准已设置] 使用首个合格品长边尺寸: " << reference_size << "px" << endl;
            }
            det.scale = current_size / reference_size;
        } else {
            det.type = "defective";
            det.angle = rect.angle;
            while (det.angle < 0) det.angle += 360.0f;
            while (det.angle >= 360.0f) det.angle -= 360.0f;

            float current_size = max(width, height);
            det.scale = reference_initialized ? (current_size / reference_size) : 1.0f;
        }

        detections.push_back(det);
    }

    return detections;
}

void ConveyorInspector::updateCounts(const vector<Detection>& detections,
                                     vector<TrackedProduct>& tracked) {
    for (auto& track : tracked) {
        if (track.counted) {
            continue;
        }

        // 需要追踪至少10帧才计数（确保是真实产品，排除噪声）
        if (track.frames_tracked < 10) {
            continue;
        }

        // 计算移动方向和距离 (当前位置 - 初始位置)
        float dx = track.centroid.x - track.initial_pos.x;
        float dy = track.centroid.y - track.initial_pos.y;
        float total_movement = sqrt(dx*dx + dy*dy);

        // 必须有足够的移动距离（至少30像素，排除静止的背景）
        if (total_movement < 30) {
            continue;
        }

        // 判断主要移动方向（仅用于显示）
        string direction = "";
        if (abs(dx) > abs(dy)) {
            direction = (dx > 0) ? "→" : "←";
        } else {
            direction = (dy > 0) ? "↓" : "↑";
        }

        // 找到对应的检测结果
        for (const auto& det : detections) {
            float dist = norm(det.centroid - track.centroid);
            if (dist < 50.0f) {
                track.counted = true;

                // 记录已统计的产品信息
                CountedProduct cp;
                cp.id = track.id;
                cp.type = det.type;
                cp.angle = det.angle;
                cp.scale = det.scale;
                cp.frame = frame_count;
                counted_products.push_back(cp);

                if (det.type == "qualified") {
                    qualified_count++;
                    cout << "Frame " << frame_count << ": ✓ QUALIFIED " << direction << " - "
                         << "ID:" << track.id << ", "
                         << "Angle: " << fixed << setprecision(1) << det.angle << "°, "
                         << "Scale: " << setprecision(2) << det.scale << "x | "
                         << "Total -> Qualified: " << qualified_count
                         << ", Defective: " << defective_count << endl;
                } else {
                    defective_count++;
                    cout << "Frame " << frame_count << ": ✗ DEFECTIVE " << direction << " - "
                         << "ID:" << track.id << ", "
                         << "Angle: " << fixed << setprecision(1) << det.angle << "°, "
                         << "Scale: " << setprecision(2) << det.scale << "x | "
                         << "Total -> Qualified: " << qualified_count
                         << ", Defective: " << defective_count << endl;
                }
                break;
            }
        }
    }
}

Mat ConveyorInspector::drawDetections(const Mat& frame, const vector<Detection>& detections,
                                       const vector<TrackedProduct>& tracked) {
    Mat result;
    frame.copyTo(result);

    // 绘制每个检测到的产品
    for (const auto& det : detections) {
        // 根据类型选择颜色
        Scalar color = (det.type == "qualified") ? Scalar(0, 255, 0) : Scalar(0, 0, 255);

        // 绘制外接矩形
        for (int i = 0; i < 4; i++) {
            line(result, det.box[i], det.box[(i+1)%4], color, 2);
        }

        // 绘制质心
        circle(result, det.centroid, 5, color, -1);

        // 显示ID、类型、角度和缩放倍数
        for (const auto& track : tracked) {
            float dist = norm(det.centroid - track.centroid);
            if (dist < 50.0f) {
                // 找到边界框的最低点（最大y坐标）
                float max_y = det.box[0].y;
                for (int i = 1; i < 4; i++) {
                    if (det.box[i].y > max_y) {
                        max_y = det.box[i].y;
                    }
                }

                // 在物体下方显示信息（从最低点下方15像素开始）
                int base_y = static_cast<int>(max_y) + 15;
                int text_x = static_cast<int>(det.centroid.x) - 50;

                // 第1行：ID和类型
                string label1 = format("ID:%d %s", track.id,
                                    det.type == "qualified" ? "YES" : "NO");
                putText(result, label1, Point(text_x, base_y + 20),
                       FONT_HERSHEY_SIMPLEX, 0.6, color, 2);

                // 第2行：旋转角度
                string label2 = format("Angle:%.1fdeg", det.angle);
                putText(result, label2, Point(text_x, base_y + 40),
                       FONT_HERSHEY_SIMPLEX, 0.5, color, 2);

                // 第3行：缩放倍数
                string label3 = format("Scale:%.2fx", det.scale);
                putText(result, label3, Point(text_x, base_y + 60),
                       FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
                break;
            }
        }
    }

    rectangle(result, Point(0, 0), Point(result.cols, 70), Scalar(0, 0, 0), -1);

    // 第1行：统计信息
    string stats = format("Frame: %d | Qualified: %d | Defective: %d | Total: %d",
                         frame_count, qualified_count, defective_count,
                         qualified_count + defective_count);
    putText(result, stats, Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.65,
           Scalar(255, 255, 255), 2);

    // 第2行：缩放基准信息（不显示合格率）
    if (reference_initialized) {
        string ref_info = format("Scale Reference: %.1fpx (1st Qualified)", reference_size);
        putText(result, ref_info, Point(10, 55), FONT_HERSHEY_SIMPLEX, 0.55,
               Scalar(100, 255, 255), 1);  // 黄色
    } else {
        string waiting = "Waiting for first qualified product to set scale reference...";
        putText(result, waiting, Point(10, 55), FONT_HERSHEY_SIMPLEX, 0.55,
               Scalar(100, 100, 255), 1);  // 橙色
    }

    return result;
}

void ConveyorInspector::processVideo(const string& video_path, bool show_video) {
    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        cerr << "错误: 无法打开视频 " << video_path << endl;
        return;
    }

    cout << "============================================================" << endl;
    cout << "Processing: " << video_path << endl;
    cout << "============================================================" << endl;
    cout << endl;

    VideoWriter video_writer;
    bool use_video_output = false;
    bool gui_available = show_video;

    if (show_video) {
        cout << "实时显示模式已启用" << endl;
        cout << "播放控制: ESC/q-退出, 空格-暂停/继续, 右方向键-加速" << endl;
        cout << "如果窗口无法显示，将自动切换到视频文件输出模式" << endl;
    }

    bool speed_boost = false;

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

        // 显示或保存视频
        if (gui_available || use_video_output) {
            Mat result = drawDetections(frame, detections, tracked);

            if (speed_boost && gui_available) {
                string speed_hint = ">> FAST FORWARD (Press Right Arrow to Normal) <<";
                putText(result, speed_hint, Point(result.cols - 600, result.rows - 20),
                       FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255), 2);  // 黄色提示
            }

            if (gui_available && !use_video_output) {
                try {
                    imshow("Product Inspection", result);

                    int delay = speed_boost ? 5 : 30;
                    int key = waitKeyEx(delay);

                    if (key == 27 || key == 'q') {  // ESC或q键退出
                        cout << "\n用户中断播放" << endl;
                        break;
                    } else if (key == 32 || key == ' ') {  // 空格键暂停
                        cout << "\n▌▌ 已暂停 (按空格继续, ESC/q退出)" << endl;

                        // 暂停循环：持续显示当前帧直到按下空格或退出
                        while (true) {
                            int pause_key = waitKey(0);  // 无限等待按键

                            if (pause_key == 32 || pause_key == ' ') {  // 空格键继续
                                cout << "▶ 继续播放" << endl;
                                break;
                            } else if (pause_key == 27 || pause_key == 'q') {  // ESC或q退出
                                cout << "\n用户中断播放" << endl;
                                goto end_video;  // 跳出外层循环
                            }
                        }
                    } else if (key == 2555904 || key == 65363) {
                        speed_boost = !speed_boost;
                        if (speed_boost) {
                            cout << "⏩ 加速播放 (再按右方向键恢复正常)" << endl;
                        } else {
                            cout << "▶ 正常播放" << endl;
                        }
                    }
                } catch (cv::Exception& e) {
                    // 第一次imshow失败，切换到视频输出
                    if (gui_available && !use_video_output) {
                        cerr << "\n警告: GUI窗口显示失败" << endl;
                        cerr << "错误: " << e.what() << endl;
                        cerr << "正在切换到视频文件输出模式...\n" << endl;

                        gui_available = false;
                        use_video_output = true;

                        // 创建输出视频
                        int fps = static_cast<int>(cap.get(CAP_PROP_FPS));
                        if (fps <= 0) fps = 30;
                        Size frame_size(result.cols, result.rows);
                        string output_path = video_path.substr(0, video_path.find_last_of('.')) + "_result.mp4";
                        video_writer.open(output_path, VideoWriter::fourcc('m','p','4','v'),
                                        fps, frame_size);

                        if (video_writer.isOpened()) {
                            cout << "输出视频: " << output_path << endl;
                            video_writer.write(result);  // 写入当前帧
                        }
                    }
                }
            }

            if (use_video_output && video_writer.isOpened()) {
                video_writer.write(result);
            }
        }
    }

end_video:  // goto标签，用于从暂停状态退出

    if (gui_available) {
        destroyAllWindows();
    }

    if (use_video_output && video_writer.isOpened()) {
        video_writer.release();
        cout << "结果视频已保存" << endl;
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

    if (reference_initialized) {
        float qualified_rate = (qualified_count + defective_count) > 0
            ? (qualified_count * 100.0f / (qualified_count + defective_count))
            : 0.0f;
        cout << "合格率:     " << fixed << setprecision(2) << qualified_rate << "%" << endl;
        cout << "缩放基准:   " << setprecision(1) << reference_size << "px (首个合格品)" << endl;
    }

    cout << "============================================================" << endl;
    cout << endl;

    // 详细产品列表（按ID排序）
    cout << "详细产品列表（按ID排序）:" << endl;
    cout << "============================================================" << endl;
    cout << left << setw(6) << "ID"
         << setw(12) << "类型"
         << setw(15) << "旋转角度"
         << setw(15) << "缩放倍数"
         << setw(10) << "检测帧" << endl;
    cout << "------------------------------------------------------------" << endl;

    // 复制并按ID排序
    vector<CountedProduct> sorted_products = counted_products;
    sort(sorted_products.begin(), sorted_products.end(),
         [](const CountedProduct& a, const CountedProduct& b) {
             return a.id < b.id;
         });

    for (const auto& prod : sorted_products) {
        // 格式化角度和缩放
        stringstream angle_str, scale_str;
        angle_str << fixed << setprecision(1) << prod.angle << "°";
        scale_str << fixed << setprecision(2) << prod.scale << "x";

        cout << left << setw(6) << prod.id
             << setw(12) << (prod.type == "qualified" ? "✓ 合格品" : "✗ 次品")
             << setw(15) << angle_str.str()
             << setw(15) << scale_str.str()
             << setw(10) << prod.frame << endl;
    }

    cout << "============================================================" << endl;
    cout << endl;
}
