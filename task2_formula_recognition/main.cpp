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
    cout << "  --debug    启用调试输出,显示识别细节" << endl;
    cout << endl;
    cout << "示例:" << endl;
    cout << "  " << program_name << " images/formula.png" << endl;
    cout << "  " << program_name << " images/formula.png --debug" << endl;
    cout << endl;
}

int main(int argc, char** argv) {
    // 检查参数
    if (argc < 2) {
        printUsage(argv[0]);
        return -1;
    }

    string image_path = argv[1];
    bool debug = false;

    // 解析选项
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--debug") {
            debug = true;
        }
    }

    // 读取图像
    Mat image = imread(image_path);
    if (image.empty()) {
        cerr << "错误: 无法读取图像: " << image_path << endl;
        return -1;
    }

    // 创建识别器并识别公式
    FormulaRecognizer recognizer(debug);
    auto result = recognizer.recognizeFormula(image);

    cout << "\n========== 识别结果 ==========" << endl;
    cout << "公式: " << result.first << endl;
    cout << "计算结果: " << result.second << endl;
    cout << "==============================\n" << endl;

    return 0;
}
