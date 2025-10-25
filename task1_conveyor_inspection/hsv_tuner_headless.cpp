/**
 * HSV颜色检测参数调节工具 (无GUI版本)
 * 生成调试图片供本地查看
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

struct DetectionParams {
    // 白色背景检测参数（检测背景后反转）
    int h_min = 0, h_max = 179;      // 色调范围（白色不限色调）
    int s_min = 0, s_max = 30;       // 低饱和度（白色饱和度很低）
    int v_min = 200, v_max = 255;    // 高明度（白色明度很高）
    int morph_kernel_size = 5;
    int morph_open_iterations = 2;
    int morph_close_iterations = 1;
    int area_threshold = 5000;
    int fill_ratio_threshold = 72;  // 72 = 0.72 * 100
};

Mat processFrame(const Mat& frame, const DetectionParams& params,
                 int& count_qualified, int& count_defective) {
    Mat hsv, mask, result;
    frame.copyTo(result);

    // HSV转换
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    // 检测白色背景
    Scalar lower_white(params.h_min, params.s_min, params.v_min);
    Scalar upper_white(params.h_max, params.s_max, params.v_max);
    inRange(hsv, lower_white, upper_white, mask);

    // 反转掩码：白色背景变黑，产品变白
    bitwise_not(mask, mask);

    // 形态学操作
    int ksize = max(1, params.morph_kernel_size | 1);
    Mat kernel = getStructuringElement(MORPH_RECT, Size(ksize, ksize));

    Mat mask_processed = mask.clone();
    morphologyEx(mask_processed, mask_processed, MORPH_OPEN, kernel,
                 Point(-1,-1), params.morph_open_iterations);
    morphologyEx(mask_processed, mask_processed, MORPH_CLOSE, kernel,
                 Point(-1,-1), params.morph_close_iterations);

    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(mask_processed.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    count_qualified = 0;
    count_defective = 0;

    // 处理每个轮廓
    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area < params.area_threshold) continue;

        // 计算轮廓逼近
        vector<Point> approx;
        approxPolyDP(contour, approx, arcLength(contour, true) * 0.04, true);

        // 获取最小外接矩形
        RotatedRect rect = minAreaRect(contour);
        float rect_area = rect.size.width * rect.size.height;
        float fill_ratio = area / rect_area;

        // 判断是否为矩形
        bool is_rect = (approx.size() == 4 && fill_ratio > params.fill_ratio_threshold / 100.0);

        // 绘制轮廓
        Scalar color = is_rect ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
        drawContours(result, vector<vector<Point>>{contour}, -1, color, 2);

        // 绘制外接矩形
        Point2f vertices[4];
        rect.points(vertices);
        for (int i = 0; i < 4; i++) {
            line(result, vertices[i], vertices[(i+1)%4], color, 1);
        }

        // 显示信息
        Point2f center = rect.center;
        string info = format("V:%lu A:%.0f F:%.2f", approx.size(), area, fill_ratio);
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
    string hsv_info = format("H:[%d,%d] S:[%d,%d] V:[%d,%d]",
                            params.h_min, params.h_max,
                            params.s_min, params.s_max,
                            params.v_min, params.v_max);
    putText(result, hsv_info, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
    putText(result, hsv_info, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 0, 0), 1);

    return result;
}

void saveDebugImages(const Mat& frame, const DetectionParams& params,
                     const string& output_dir, int frame_num) {
    Mat hsv, mask;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    // 检测白色背景
    Scalar lower_white(params.h_min, params.s_min, params.v_min);
    Scalar upper_white(params.h_max, params.s_max, params.v_max);
    inRange(hsv, lower_white, upper_white, mask);

    // 反转掩码
    bitwise_not(mask, mask);

    // 形态学操作
    int ksize = max(1, params.morph_kernel_size | 1);
    Mat kernel = getStructuringElement(MORPH_RECT, Size(ksize, ksize));
    Mat mask_processed = mask.clone();
    morphologyEx(mask_processed, mask_processed, MORPH_OPEN, kernel,
                 Point(-1,-1), params.morph_open_iterations);
    morphologyEx(mask_processed, mask_processed, MORPH_CLOSE, kernel,
                 Point(-1,-1), params.morph_close_iterations);

    int count_q, count_d;
    Mat result = processFrame(frame, params, count_q, count_d);

    // 保存图片
    imwrite(output_dir + format("/frame_%03d_original.jpg", frame_num), frame);
    imwrite(output_dir + format("/frame_%03d_mask.jpg", frame_num), mask);
    imwrite(output_dir + format("/frame_%03d_processed.jpg", frame_num), mask_processed);
    imwrite(output_dir + format("/frame_%03d_result.jpg", frame_num), result);
}

void printParams(const DetectionParams& params) {
    cout << "\n=== 当前参数 (白色背景检测) ===" << endl;
    cout << "HSV 范围 (检测白色背景后反转):" << endl;
    cout << "  H: [" << params.h_min << ", " << params.h_max << "]" << endl;
    cout << "  S: [" << params.s_min << ", " << params.s_max << "]" << endl;
    cout << "  V: [" << params.v_min << ", " << params.v_max << "]" << endl;
    cout << "形态学参数:" << endl;
    cout << "  Kernel Size: " << params.morph_kernel_size << endl;
    cout << "  Open Iterations: " << params.morph_open_iterations << endl;
    cout << "  Close Iterations: " << params.morph_close_iterations << endl;
    cout << "过滤参数:" << endl;
    cout << "  Area Threshold: " << params.area_threshold << endl;
    cout << "  Fill Ratio Threshold: " << params.fill_ratio_threshold << "%" << endl;
    cout << "\n代码格式:" << endl;
    cout << "// 检测白色背景" << endl;
    cout << "Scalar lower_white(" << params.h_min << ", " << params.s_min
         << ", " << params.v_min << ");" << endl;
    cout << "Scalar upper_white(" << params.h_max << ", " << params.s_max
         << ", " << params.v_max << ");" << endl;
    cout << "inRange(hsv, lower_white, upper_white, mask);" << endl;
    cout << "bitwise_not(mask, mask);  // 反转掩码" << endl;
    cout << "Mat kernel = getStructuringElement(MORPH_RECT, Size("
         << params.morph_kernel_size << ", " << params.morph_kernel_size << "));" << endl;
    cout << "morphologyEx(mask, mask, MORPH_OPEN, kernel, Point(-1,-1), "
         << params.morph_open_iterations << ");" << endl;
    cout << "morphologyEx(mask, mask, MORPH_CLOSE, kernel, Point(-1,-1), "
         << params.morph_close_iterations << ");" << endl;
    cout << "if (area < " << params.area_threshold << ") continue;" << endl;
    cout << "if (approx.size() == 4 && area_ratio > "
         << params.fill_ratio_threshold / 100.0 << ")" << endl;
    cout << endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "用法: " << argv[0] << " <视频路径> [输出目录]" << endl;
        cout << "示例: " << argv[0] << " ../video/2.mp4 debug_output" << endl;
        return -1;
    }

    string video_path = argv[1];
    string output_dir = argc > 2 ? argv[2] : "debug_frames";

    // 创建输出目录
    system(("mkdir -p " + output_dir).c_str());

    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        cerr << "错误: 无法打开视频 " << video_path << endl;
        return -1;
    }

    cout << "=== HSV 参数调节工具 (无GUI版本) ===" << endl;
    cout << "输出目录: " << output_dir << endl;
    cout << endl;

    DetectionParams params;

    // 保存一些关键帧
    vector<int> key_frames = {0, 50, 100, 150, 200, 250, 300, 350, 370};

    int total_qualified = 0, total_defective = 0;

    for (int target_frame : key_frames) {
        cap.set(CAP_PROP_POS_FRAMES, target_frame);
        Mat frame;
        cap >> frame;

        if (frame.empty()) continue;

        int count_q, count_d;
        saveDebugImages(frame, params, output_dir, target_frame);

        cout << "处理帧 " << target_frame << " - 保存到 " << output_dir << endl;
    }

    // 统计整个视频
    cap.set(CAP_PROP_POS_FRAMES, 0);
    Mat frame;
    int frame_count = 0;

    while (cap.read(frame)) {
        int count_q, count_d;
        processFrame(frame, params, count_q, count_d);
        frame_count++;
    }

    printParams(params);

    cout << "\n=== 完成 ===" << endl;
    cout << "调试图片已保存到: " << output_dir << "/" << endl;
    cout << "关键帧: frame_XXX_*.jpg" << endl;
    cout << "  - original.jpg: 原始帧" << endl;
    cout << "  - mask.jpg: HSV掩码" << endl;
    cout << "  - processed.jpg: 形态学处理后" << endl;
    cout << "  - result.jpg: 最终检测结果" << endl;
    cout << "\n如需调整参数，请编辑代码中的 DetectionParams 结构体" << endl;

    return 0;
}
