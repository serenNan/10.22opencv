/**
 * HSV颜色检测参数调节工具
 * 使用滑动条实时调节HSV阈值、形态学参数等
 */

#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// 全局变量存储参数
int h_min = 35, h_max = 85;
int s_min = 40, s_max = 255;
int v_min = 40, v_max = 255;
int morph_kernel_size = 5;
int morph_open_iterations = 2;
int morph_close_iterations = 1;
int area_threshold = 5000;
int fill_ratio_threshold = 72;  // 填充度阈值 (72 = 0.72 * 100)

Mat current_frame;
VideoCapture cap;

void processFrame() {
    if (current_frame.empty()) return;

    Mat hsv, mask, result;
    current_frame.copyTo(result);

    // HSV转换
    cvtColor(current_frame, hsv, COLOR_BGR2HSV);

    // 颜色检测
    Scalar lower_green(h_min, s_min, v_min);
    Scalar upper_green(h_max, s_max, v_max);
    inRange(hsv, lower_green, upper_green, mask);

    // 形态学操作
    int ksize = max(1, morph_kernel_size | 1);  // 确保是奇数
    Mat kernel = getStructuringElement(MORPH_RECT, Size(ksize, ksize));

    Mat mask_processed = mask.clone();
    morphologyEx(mask_processed, mask_processed, MORPH_OPEN, kernel,
                 Point(-1,-1), morph_open_iterations);
    morphologyEx(mask_processed, mask_processed, MORPH_CLOSE, kernel,
                 Point(-1,-1), morph_close_iterations);

    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(mask_processed.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    int count_qualified = 0, count_defective = 0;

    // 处理每个轮廓
    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area < area_threshold) continue;

        // 计算轮廓逼近
        vector<Point> approx;
        approxPolyDP(contour, approx, arcLength(contour, true) * 0.04, true);

        // 获取最小外接矩形
        RotatedRect rect = minAreaRect(contour);
        float rect_area = rect.size.width * rect.size.height;
        float fill_ratio = area / rect_area;

        // 判断是否为矩形
        bool is_rect = (approx.size() == 4 && fill_ratio > fill_ratio_threshold / 100.0);

        // 绘制轮廓
        Scalar color = is_rect ? Scalar(0, 255, 0) : Scalar(0, 0, 255);  // 绿色=合格，红色=次品
        drawContours(result, vector<vector<Point>>{contour}, -1, color, 2);

        // 绘制外接矩形
        Point2f vertices[4];
        rect.points(vertices);
        for (int i = 0; i < 4; i++) {
            line(result, vertices[i], vertices[(i+1)%4], color, 1);
        }

        // 显示信息
        Point2f center = rect.center;
        string info = format("V:%d A:%.0f F:%.2f", approx.size(), area, fill_ratio);
        putText(result, info, Point(center.x - 40, center.y - 10),
                FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

        if (is_rect) count_qualified++;
        else count_defective++;
    }

    // 显示统计信息
    string stats = format("Qualified: %d  Defective: %d  Total: %d",
                         count_qualified, count_defective, count_qualified + count_defective);
    putText(result, stats, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
    putText(result, stats, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 0), 1);

    // 显示HSV参数
    string hsv_info = format("H:[%d,%d] S:[%d,%d] V:[%d,%d]", h_min, h_max, s_min, s_max, v_min, v_max);
    putText(result, hsv_info, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
    putText(result, hsv_info, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 0, 0), 1);

    // 显示窗口
    imshow("Original", current_frame);
    imshow("Mask", mask);
    imshow("Processed Mask", mask_processed);
    imshow("Result", result);
}

void onTrackbar(int, void*) {
    processFrame();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "用法: " << argv[0] << " <视频路径>" << endl;
        cout << "示例: " << argv[0] << " ../video/2.mp4" << endl;
        return -1;
    }

    string video_path = argv[1];
    cap.open(video_path);

    if (!cap.isOpened()) {
        cerr << "错误: 无法打开视频 " << video_path << endl;
        return -1;
    }

    // 读取第一帧
    cap >> current_frame;
    if (current_frame.empty()) {
        cerr << "错误: 无法读取视频帧" << endl;
        return -1;
    }

    // 创建窗口
    namedWindow("Original", WINDOW_NORMAL);
    namedWindow("Mask", WINDOW_NORMAL);
    namedWindow("Processed Mask", WINDOW_NORMAL);
    namedWindow("Result", WINDOW_NORMAL);
    namedWindow("Controls", WINDOW_NORMAL);

    // 调整窗口大小
    resizeWindow("Original", 640, 480);
    resizeWindow("Mask", 640, 480);
    resizeWindow("Processed Mask", 640, 480);
    resizeWindow("Result", 640, 480);
    resizeWindow("Controls", 600, 400);

    // 创建滑动条
    createTrackbar("H Min", "Controls", &h_min, 179, onTrackbar);
    createTrackbar("H Max", "Controls", &h_max, 179, onTrackbar);
    createTrackbar("S Min", "Controls", &s_min, 255, onTrackbar);
    createTrackbar("S Max", "Controls", &s_max, 255, onTrackbar);
    createTrackbar("V Min", "Controls", &v_min, 255, onTrackbar);
    createTrackbar("V Max", "Controls", &v_max, 255, onTrackbar);
    createTrackbar("Kernel Size", "Controls", &morph_kernel_size, 15, onTrackbar);
    createTrackbar("Open Iterations", "Controls", &morph_open_iterations, 5, onTrackbar);
    createTrackbar("Close Iterations", "Controls", &morph_close_iterations, 5, onTrackbar);
    createTrackbar("Area Threshold/100", "Controls", &area_threshold, 500, onTrackbar);
    createTrackbar("Fill Ratio %", "Controls", &fill_ratio_threshold, 100, onTrackbar);

    // 初始显示
    processFrame();

    cout << "=== HSV 颜色检测参数调节工具 ===" << endl;
    cout << "操作说明:" << endl;
    cout << "  - 使用滑动条调节参数" << endl;
    cout << "  - 按 SPACE 切换到下一帧" << endl;
    cout << "  - 按 'r' 重置到第一帧" << endl;
    cout << "  - 按 'p' 打印当前参数" << endl;
    cout << "  - 按 'q' 或 ESC 退出" << endl;
    cout << endl;

    int frame_count = 0;

    while (true) {
        int key = waitKey(30);

        if (key == 27 || key == 'q') {  // ESC 或 q
            break;
        } else if (key == ' ') {  // 空格键 - 下一帧
            cap >> current_frame;
            if (current_frame.empty()) {
                cout << "已到视频末尾，重置到开头" << endl;
                cap.set(CAP_PROP_POS_FRAMES, 0);
                cap >> current_frame;
                frame_count = 0;
            } else {
                frame_count++;
            }
            processFrame();
            cout << "Frame: " << frame_count << endl;
        } else if (key == 'r') {  // r - 重置到第一帧
            cap.set(CAP_PROP_POS_FRAMES, 0);
            cap >> current_frame;
            frame_count = 0;
            processFrame();
            cout << "重置到第一帧" << endl;
        } else if (key == 'p') {  // p - 打印参数
            cout << "\n=== 当前参数 ===" << endl;
            cout << "HSV 范围:" << endl;
            cout << "  H: [" << h_min << ", " << h_max << "]" << endl;
            cout << "  S: [" << s_min << ", " << s_max << "]" << endl;
            cout << "  V: [" << v_min << ", " << v_max << "]" << endl;
            cout << "形态学参数:" << endl;
            cout << "  Kernel Size: " << morph_kernel_size << endl;
            cout << "  Open Iterations: " << morph_open_iterations << endl;
            cout << "  Close Iterations: " << morph_close_iterations << endl;
            cout << "过滤参数:" << endl;
            cout << "  Area Threshold: " << area_threshold << endl;
            cout << "  Fill Ratio Threshold: " << fill_ratio_threshold << "%" << endl;
            cout << "\n代码格式:" << endl;
            cout << "Scalar lower_green(" << h_min << ", " << s_min << ", " << v_min << ");" << endl;
            cout << "Scalar upper_green(" << h_max << ", " << s_max << ", " << v_max << ");" << endl;
            cout << "Mat kernel = getStructuringElement(MORPH_RECT, Size(" << morph_kernel_size
                 << ", " << morph_kernel_size << "));" << endl;
            cout << "morphologyEx(mask, mask, MORPH_OPEN, kernel, Point(-1,-1), "
                 << morph_open_iterations << ");" << endl;
            cout << "morphologyEx(mask, mask, MORPH_CLOSE, kernel, Point(-1,-1), "
                 << morph_close_iterations << ");" << endl;
            cout << "if (area < " << area_threshold << ") continue;" << endl;
            cout << "if (approx.size() == 4 && area_ratio > " << fill_ratio_threshold / 100.0 << ")" << endl;
            cout << endl;
        }
    }

    cap.release();
    destroyAllWindows();

    return 0;
}
