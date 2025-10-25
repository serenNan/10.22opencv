/**
 * 公式识别系统 - 实现文件
 * 实现基于形态学特征的字符识别功能
 */

#include "formula_recognizer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stack>
#include <cmath>

// ============================================================================
// RecognizedChar 实现
// ============================================================================

RecognizedChar::RecognizedChar(char c, Rect box, float conf)
    : character(c), boundingBox(box), confidence(conf) {}

bool RecognizedChar::operator<(const RecognizedChar& other) const {
    return boundingBox.x < other.boundingBox.x;
}

// ============================================================================
// FormulaRecognizer 类实现
// ============================================================================

FormulaRecognizer::FormulaRecognizer(bool enableDebug)
    : debug(enableDebug) {}

Mat FormulaRecognizer::preprocessImage(const Mat& input) {
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

char FormulaRecognizer::recognizeCharacter(const Mat& roi, const Rect& box) {
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

    char result = '?';

    // 运算符识别
    if (h < 20 && aspectRatio > 1.8 && density > 0.4) {
        result = '=';
    } else if (aspectRatio > 2.5 && h < 10) {
        result = '-';
    } else if (aspectRatio > 0.85 && aspectRatio < 1.15 && density > 0.2 && density < 0.35) {
        result = '+';
    } else if (aspectRatio > 0.8 && aspectRatio < 1.2 && density > 0.3 && density < 0.6) {
        result = 'x';
    } else if (numHoles >= 2 && aspectRatio > 0.6 && aspectRatio < 1.4) {
        result = '/';
    }
    // 数字识别
    else if (aspectRatio < 0.65 && density < 0.43 && numHoles == 0) {
        result = '1';
    } else if (numHoles >= 2 || (density > 0.65 && pixelCount > 650)) {
        result = '8';
    } else if (numHoles >= 1 && density > 0.5 && (botRatio > 0.36 || midRatio > 0.36)) {
        result = '6';
    } else if (numHoles >= 1 && topRatio > 0.4 && density > 0.5) {
        result = '9';
    } else if (numHoles >= 1 && midRatio > 0.3 && density > 0.42 && density < 0.5) {
        result = '4';
    } else if (numHoles >= 1 && aspectRatio > 0.65 && topRatio > 0.25 && botRatio > 0.25) {
        result = '0';
    } else if (numHoles == 0 && midRatio > 0.28 && topRatio > 0.3 && botRatio > 0.3 && density > 0.45) {
        result = '3';
    } else if (numHoles == 0 && topRatio > 0.3 && botRatio > 0.35 && midRatio < 0.25 && density > 0.4 && density < 0.55) {
        result = '2';
    } else if (topRatio > 0.45 && density > 0.4 && density < 0.6) {
        result = '5';
    } else if (topRatio > 0.5 && aspectRatio < 0.7 && density < 0.45) {
        result = '7';
    } else if (numHoles == 0) {
        if (aspectRatio < 0.65 && density < 0.43) result = '1';
        else if (midRatio > 0.28 && density > 0.45) result = '3';
        else if (midRatio < 0.25 && density > 0.4) result = '2';
    } else if (numHoles >= 1 && density < 0.5) {
        result = '4';
    } else if (numHoles >= 1) {
        result = '6';
    }

    if (debug) {
        cout << "      [" << result << "] 位置(" << box.x << "," << box.y << ") "
             << w << "x" << h << " AR:" << fixed << setprecision(2) << aspectRatio
             << " D:" << density << " H:" << numHoles
             << " TMB:" << topRatio << ":" << midRatio << ":" << botRatio << endl;
    }

    return result;
}

vector<RecognizedChar> FormulaRecognizer::detectCharacters(const Mat& binary) {
    vector<RecognizedChar> characters;

    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(binary.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 获取边界框
    vector<Rect> boxes;
    for (const auto& contour : contours) {
        Rect box = boundingRect(contour);
        if (box.width < 3 || box.height < 3 || box.area() < 15) {
            continue;
        }
        boxes.push_back(box);
    }

    // 按x坐标排序
    sort(boxes.begin(), boxes.end(),
         [](const Rect& a, const Rect& b) { return a.x < b.x; });

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

        if (box.height < 5 && box.width > box.height * 3) {
            if (i + 1 < boxes.size()) {
                Rect nextBox = boxes[i + 1];
                int xDiff = abs(box.x - nextBox.x);
                int yDiff = abs(box.y - nextBox.y);

                if (nextBox.height < 5 && nextBox.width > nextBox.height * 3 &&
                    xDiff < 10 && yDiff > 3 && yDiff < 15) {
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

double FormulaRecognizer::evaluateExpression(const string& expr) {
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
    try {
        stack<double> numbers;
        stringstream ss(expression);
        double num;

        if (ss >> num) {
            numbers.push(num);
        } else {
            return 0.0;
        }

        char op;
        while (ss >> op) {
            if (ss >> num) {
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

pair<string, double> FormulaRecognizer::recognizeFormula(const Mat& image) {
    cout << "开始图像预处理..." << endl;

    Mat binary = preprocessImage(image);

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

    double result = evaluateExpression(expression);

    return make_pair(expression, result);
}
