/*
 * 公式识别系统 (C++ CLI版本)
 * 功能: 识别图片中的数学公式并计算结果
 * 支持: 数字0-9, 运算符 +, -, ×, ÷, =
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stack>

using namespace cv;
using namespace std;

// 字符识别结构体
struct RecognizedChar {
    char character;
    Rect boundingBox;
    float confidence;

    RecognizedChar(char c, Rect box, float conf = 1.0)
        : character(c), boundingBox(box), confidence(conf) {}

    bool operator<(const RecognizedChar& other) const {
        return boundingBox.x < other.boundingBox.x;
    }
};

// 公式识别器类
class FormulaRecognizer {
private:
    bool debug;

    // 图像预处理
    Mat preprocessImage(const Mat& input) {
        Mat gray, binary;

        // 转换为灰度图
        if (input.channels() == 3) {
            cvtColor(input, gray, COLOR_BGR2GRAY);
        } else {
            gray = input.clone();
        }

        // 使用Otsu阈值二值化
        threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

        // 去噪
        Mat kernel = getStructuringElement(MORPH_RECT, Size(2, 2));
        morphologyEx(binary, binary, MORPH_CLOSE, kernel);

        if (debug) {
            imwrite("debug_binary.png", binary);
        }

        return binary;
    }

    // 识别单个字符
    char recognizeCharacter(const Mat& roi, const Rect& box) {
        int h = roi.rows;
        int w = roi.cols;

        if (h == 0 || w == 0) return '?';

        // 归一化大小
        Mat resized;
        resize(roi, resized, Size(28, 40));

        // 计算特征
        int pixelCount = countNonZero(resized);
        float density = (float)pixelCount / (resized.rows * resized.cols);
        float aspectRatio = (float)w / h;

        // 查找轮廓
        vector<vector<Point>> contours;
        findContours(resized.clone(), contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
        int numHoles = max(0, (int)contours.size() - 1);

        // 计算上中下三部分的像素分布
        int h1 = resized.rows / 3;
        int h2 = resized.rows * 2 / 3;

        Mat topPart = resized(Rect(0, 0, resized.cols, h1));
        Mat midPart = resized(Rect(0, h1, resized.cols, h2 - h1));
        Mat botPart = resized(Rect(0, h2, resized.cols, resized.rows - h2));

        int topPixels = countNonZero(topPart);
        int midPixels = countNonZero(midPart);
        int botPixels = countNonZero(botPart);
        int totalPixels = topPixels + midPixels + botPixels;

        if (totalPixels == 0) return '?';

        float topRatio = (float)topPixels / totalPixels;
        float midRatio = (float)midPixels / totalPixels;
        float botRatio = (float)botPixels / totalPixels;

        // 保存识别结果用于调试输出
        char result = '?';

        // 运算符识别
        // 等号 - 两条横线（降低宽高比要求，因为合并后的等号可能不够宽）
        if (h < 20 && aspectRatio > 1.8 && density > 0.4) {
            result = '=';
        }
        // 减号 - 横线（单条横线，宽高比更大）
        else if (aspectRatio > 2.5 && h < 10) {
            result = '-';
        }
        // 加号 - 十字形,接近方形
        else if (aspectRatio > 0.85 && aspectRatio < 1.15 && density > 0.2 && density < 0.35) {
            result = '+';
        }
        // 乘号 - X形
        else if (aspectRatio > 0.8 && aspectRatio < 1.2 && density > 0.3 && density < 0.6) {
            result = 'x';
        }
        // 除号
        else if (numHoles >= 2 && aspectRatio > 0.6 && aspectRatio < 1.4) {
            result = '/';
        }

        // 数字识别 - 基于实际测试数据优化的规则
        else if (aspectRatio < 0.65 && density < 0.43 && numHoles == 0) {
            result = '1';  // 窄而高，密度小，无洞
        }
        else if (numHoles >= 2 || (density > 0.65 && pixelCount > 650)) {
            result = '8';  // 两个洞或像素很多
        }
        else if (numHoles >= 1 && density > 0.5 && (botRatio > 0.36 || midRatio > 0.36)) {
            result = '6';  // 一个洞，下部重或中下部重
        }
        else if (numHoles >= 1 && topRatio > 0.4 && density > 0.5) {
            result = '9';  // 一个洞,上部重，密度高
        }
        else if (numHoles >= 1 && midRatio > 0.3 && density > 0.42 && density < 0.5) {
            result = '4';  // 有洞，中间部分较多，密度适中
        }
        else if (numHoles >= 1 && aspectRatio > 0.65 && topRatio > 0.25 && botRatio > 0.25) {
            result = '0';  // 有洞,宽一点,上下均匀
        }
        else if (numHoles == 0 && midRatio > 0.28 && topRatio > 0.3 && botRatio > 0.3 && density > 0.45) {
            result = '3';  // 无洞，中间部分多，上下也有
        }
        else if (numHoles == 0 && topRatio > 0.3 && botRatio > 0.35 && midRatio < 0.25 && density > 0.4 && density < 0.55) {
            result = '2';  // 无洞，上下都有，中间少，密度中等
        }
        else if (topRatio > 0.45 && density > 0.4 && density < 0.6) {
            result = '5';  // 上部多，密度中等
        }
        else if (topRatio > 0.5 && aspectRatio < 0.7 && density < 0.45) {
            result = '7';  // 顶部横线，窄，密度小
        }
        // 默认根据特征判断
        else if (numHoles == 0) {
            if (aspectRatio < 0.65 && density < 0.43) result = '1';
            else if (midRatio > 0.28 && density > 0.45) result = '3';
            else if (midRatio < 0.25 && density > 0.4) result = '2';
        }
        else if (numHoles >= 1 && density < 0.5) {
            result = '4';
        }
        else if (numHoles >= 1) {
            result = '6';
        }

        // 调试输出：显示特征值和识别结果
        if (debug) {
            cout << "      [" << result << "] 位置(" << box.x << "," << box.y << ") "
                 << w << "x" << h << " AR:" << fixed << setprecision(2) << aspectRatio
                 << " D:" << density << " H:" << numHoles
                 << " TMB:" << topRatio << ":" << midRatio << ":" << botRatio << endl;
        }

        return result;
    }

    // 检测字符
    vector<RecognizedChar> detectCharacters(const Mat& binary) {
        vector<RecognizedChar> characters;

        // 查找轮廓
        vector<vector<Point>> contours;
        findContours(binary.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // 获取边界框
        vector<Rect> boxes;
        for (const auto& contour : contours) {
            Rect box = boundingRect(contour);

            // 过滤噪声
            if (box.width < 3 || box.height < 3 || box.area() < 15) {
                continue;
            }

            boxes.push_back(box);
        }

        // 按x坐标排序
        sort(boxes.begin(), boxes.end(),
             [](const Rect& a, const Rect& b) { return a.x < b.x; });

        // 调试输出原始边界框
        if (debug) {
            cout << "  [调试] 检测到 " << boxes.size() << " 个原始边界框:" << endl;
            for (size_t i = 0; i < boxes.size(); i++) {
                cout << "    Box " << i << ": (" << boxes[i].x << "," << boxes[i].y
                     << ") " << boxes[i].width << "x" << boxes[i].height << endl;
            }
        }

        // 合并等号的两条横线
        vector<Rect> mergedBoxes;
        size_t i = 0;
        while (i < boxes.size()) {
            Rect box = boxes[i];

            // 检查是否是等号的一部分(矮而宽的矩形)
            if (box.height < 5 && box.width > box.height * 3) {
                // 查找下一个可能的等号部分
                if (i + 1 < boxes.size()) {
                    Rect nextBox = boxes[i + 1];
                    int xDiff = abs(box.x - nextBox.x);
                    int yDiff = abs(box.y - nextBox.y);

                    if (nextBox.height < 5 && nextBox.width > nextBox.height * 3 &&
                        xDiff < 10 && yDiff > 3 && yDiff < 15) {
                        // 合并为等号
                        int minX = min(box.x, nextBox.x);
                        int minY = min(box.y, nextBox.y);
                        int maxX = max(box.br().x, nextBox.br().x);
                        int maxY = max(box.br().y, nextBox.br().y);

                        mergedBoxes.push_back(Rect(minX, minY, maxX - minX, maxY - minY));
                        i += 2;
                        continue;
                    }
                }
            }

            mergedBoxes.push_back(box);
            i++;
        }

        // 识别每个字符
        for (const auto& box : mergedBoxes) {
            Mat roi = binary(box);
            char recognized = recognizeCharacter(roi, box);

            if (recognized != '?') {
                characters.push_back(RecognizedChar(recognized, box));
            }
        }

        return characters;
    }

    // 计算表达式
    double evaluateExpression(const string& expr) {
        string expression = expr;

        // 移除等号后的部分
        size_t eqPos = expression.find('=');
        if (eqPos != string::npos) {
            expression = expression.substr(0, eqPos);
        }

        // 替换运算符
        for (size_t i = 0; i < expression.length(); i++) {
            if (expression[i] == 'x') expression[i] = '*';
        }

        // 简单的表达式求值
        // 支持 +, -, *, /
        try {
            // 使用栈进行简单计算
            stack<double> numbers;
            stack<char> operators;

            stringstream ss(expression);
            double num;
            char op;

            // 读取第一个数字
            if (ss >> num) {
                numbers.push(num);
            } else {
                return 0.0;
            }

            // 读取运算符和数字
            while (ss >> op) {
                if (ss >> num) {
                    // 立即计算(简化版,只支持左到右)
                    double left = numbers.top();
                    numbers.pop();

                    double result;
                    switch (op) {
                        case '+': result = left + num; break;
                        case '-': result = left - num; break;
                        case '*': result = left * num; break;
                        case '/': result = (num != 0) ? left / num : 0; break;
                        default: result = left; break;
                    }
                    numbers.push(result);
                }
            }

            if (!numbers.empty()) {
                return numbers.top();
            }
        } catch (...) {
            return 0.0;
        }

        return 0.0;
    }

public:
    FormulaRecognizer(bool enableDebug = false) : debug(enableDebug) {}

    // 识别公式
    pair<string, double> recognizeFormula(const Mat& image) {
        cout << "开始图像预处理..." << endl;

        // 预处理
        Mat binary = preprocessImage(image);

        // 检测字符
        cout << "正在检测字符..." << endl;
        vector<RecognizedChar> chars = detectCharacters(binary);

        if (chars.empty()) {
            cout << "警告: 未检测到任何字符!" << endl;
            return make_pair("", 0.0);
        }

        cout << "检测到 " << chars.size() << " 个字符" << endl;

        // 构建表达式
        string expression;
        for (const auto& ch : chars) {
            expression += ch.character;
            if (debug) {
                cout << "  字符: " << ch.character
                     << " 位置: (" << ch.boundingBox.x << ", " << ch.boundingBox.y << ")"
                     << " 大小: " << ch.boundingBox.width << "x" << ch.boundingBox.height
                     << endl;
            }
        }

        cout << "识别的字符序列: " << expression << endl;

        // 计算结果
        double result = evaluateExpression(expression);

        return make_pair(expression, result);
    }
};

int main(int argc, char** argv) {
    cout << "========================================" << endl;
    cout << "  公式识别系统 v1.0 (C++ CLI版本)     " << endl;
    cout << "========================================" << endl;
    cout << endl;

    if (argc < 2) {
        cout << "用法: " << argv[0] << " <图片路径> [--debug]" << endl;
        cout << "示例: " << argv[0] << " formula_images/formula_1.png" << endl;
        return -1;
    }

    string imagePath = argv[1];
    bool debug = (argc > 2 && string(argv[2]) == "--debug");

    // 读取图像
    Mat image = imread(imagePath);
    if (image.empty()) {
        cerr << "错误: 无法读取图像 " << imagePath << endl;
        return -1;
    }

    cout << "图像尺寸: " << image.cols << " x " << image.rows << endl;
    cout << endl;

    // 创建识别器
    FormulaRecognizer recognizer(debug);

    // 识别公式
    auto result = recognizer.recognizeFormula(image);
    string expression = result.first;
    double value = result.second;

    // 输出结果
    cout << endl;
    cout << "========================================" << endl;
    cout << "           识别结果                     " << endl;
    cout << "========================================" << endl;

    if (!expression.empty()) {
        cout << "识别的表达式: " << expression << endl;

        // 分离等号前后
        size_t eqPos = expression.find('=');
        if (eqPos != string::npos) {
            string leftPart = expression.substr(0, eqPos);
            string rightPart = expression.substr(eqPos + 1);
            cout << "计算结果: " << leftPart << " = " << value << endl;

            // 如果有预期结果,进行比较
            if (!rightPart.empty()) {
                try {
                    double expected = stod(rightPart);
                    cout << "预期结果: " << expected << endl;
                    if (abs(value - expected) < 0.01) {
                        cout << "✓ 结果正确!" << endl;
                    } else {
                        cout << "✗ 结果不匹配" << endl;
                    }
                } catch (...) {
                    // 无法解析预期结果
                }
            }
        } else {
            cout << "计算结果: " << expression << " = " << value << endl;
        }
    } else {
        cout << "未识别到任何公式" << endl;
    }

    cout << "========================================" << endl;

    return 0;
}
