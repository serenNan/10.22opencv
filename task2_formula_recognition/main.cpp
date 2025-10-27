/**
 * 公式识别系统 - 主程序
 * 命令行入口,处理参数并调用识别器
 */

#include "formula_recognizer.h"
#include <iostream>

using namespace std;

void printUsage(const char* program_name) {
    cout << "公式识别系统 v1.0" << endl;
    cout << endl;
    cout << "用法: " << program_name << " <图像路径> [选项]" << endl;
    cout << endl;
    cout << "选项:" << endl;
    cout << "  --output <路径>  将结果写入图片并保存" << endl;
    cout << "  --single        强制单公式识别模式(默认自动检测多公式)" << endl;
    cout << endl;
    cout << "示例:" << endl;
    cout << "  " << program_name << " images/formula.png" << endl;
    cout << "  " << program_name << " images/formula.png --output result.png" << endl;
    cout << "  " << program_name << " images/formula.png --single  # 强制单公式模式" << endl;
    cout << endl;
}

int main(int argc, char** argv) {
    // 检查参数
    if (argc < 2) {
        printUsage(argv[0]);
        return -1;
    }

    string image_path = argv[1];
    string output_path = "";
    bool multi_mode = true;  // 默认启用多公式模式

    // 解析选项
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            output_path = argv[i + 1];
            i++;  // 跳过下一个参数
        } else if (arg == "--single") {
            multi_mode = false;
        }
    }

    // 读取图像
    Mat image = imread(image_path);
    if (image.empty()) {
        cerr << "错误: 无法读取图像: " << image_path << endl;
        return -1;
    }

    // 创建识别器
    FormulaRecognizer recognizer;

    // 默认生成输出图片路径
    if (output_path.empty()) {
        // 自动生成输出文件名: 原文件名_result.png
        size_t lastSlash = image_path.find_last_of("/\\");
        size_t lastDot = image_path.find_last_of(".");

        string baseDir = (lastSlash != string::npos) ? image_path.substr(0, lastSlash + 1) : "";
        string baseName = (lastSlash != string::npos) ? image_path.substr(lastSlash + 1) : image_path;

        if (lastDot != string::npos && lastDot > lastSlash) {
            baseName = baseName.substr(0, lastDot - (lastSlash != string::npos ? lastSlash + 1 : 0));
        }

        output_path = baseDir + baseName + "_result.png";
    }

    // 根据模式进行识别
    if (multi_mode) {
        // 多公式识别模式
        vector<FormulaResult> results = recognizer.recognizeMultipleFormulas(image);

        cout << "\n========== 识别结果 ==========" << endl;
        for (size_t i = 0; i < results.size(); i++) {
            cout << "公式 " << (i + 1) << ": " << results[i].expression << endl;
            cout << "计算结果: " << results[i].result << endl;
            cout << "------------------------------" << endl;
        }
        cout << "==============================\n" << endl;

        // 将所有结果写入图片
        recognizer.writeMultipleResultsToImage(image, results, output_path);
        cout << "✓ 结果已写入图片: " << output_path << endl;

    } else {
        // 单公式识别模式
        auto result = recognizer.recognizeFormula(image);

        cout << "\n========== 识别结果 ==========" << endl;
        cout << "公式: " << result.first << endl;
        cout << "计算结果: " << result.second << endl;
        cout << "==============================\n" << endl;

        // 将结果写入图片
        recognizer.writeResultToImage(image, result.first, result.second, output_path);
        cout << "✓ 结果已写入图片: " << output_path << endl;
    }

    // 只在有显示环境时显示窗口
    const char* display = getenv("DISPLAY");
    if (display != nullptr && strlen(display) > 0) {
        // 读取并显示结果图片
        Mat resultImage = imread(output_path);
        if (!resultImage.empty()) {
            namedWindow("公式识别结果", WINDOW_AUTOSIZE);
            imshow("公式识别结果", resultImage);
            cout << "\n按任意键关闭窗口..." << endl;
            waitKey(0);
            destroyAllWindows();
        }
    }

    return 0;
}
