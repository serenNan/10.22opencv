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
#include <algorithm>

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

FormulaRecognizer::FormulaRecognizer() {}

Mat FormulaRecognizer::preprocessImage(const Mat& input) {
    Mat gray, binary;

    if (input.channels() == 3) {
        cvtColor(input, gray, COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }

    threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

    Mat kernel = getStructuringElement(MORPH_RECT, Size(2, 2));
    morphologyEx(binary, binary, MORPH_CLOSE, kernel);

    return binary;
}

char FormulaRecognizer::recognizeCharacter(const Mat& roi, const Rect& box) {
    int h = roi.rows;
    int w = roi.cols;

    if (h == 0 || w == 0) return '?';

    Mat resized;
    resize(roi, resized, Size(28, 40));

    int pixelCount = countNonZero(resized);
    float density = (float)pixelCount / (resized.rows * resized.cols);
    float aspectRatio = (float)w / h;

    vector<vector<Point>> contours;
    findContours(resized.clone(), contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
    int numHoles = max(0, (int)contours.size() - 1);

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

    // 运算符和括号识别 - 优先判断,避免与数字混淆

    // 根号识别: 密度很低,中下部较重,顶部轻
    // 特征: AR:0.6-0.8, D:0.15-0.28, H:0, 中下部明显重于顶部
    // 与数字5区分: 根号密度更低,顶部更轻
    if (aspectRatio > 0.6 && aspectRatio < 0.85 && density > 0.15 && density < 0.28 &&
        numHoles == 0 && h > 35 && topRatio < 0.25 && (midRatio + botRatio) > 0.70) {
        result = 's';  // 用's'表示sqrt根号
    }
    // 括号识别: 细长,密度中等,无孔洞
    // 左括号(: aspectRatio小,密度0.45-0.55
    // 右括号): aspectRatio小,密度0.45-0.55
    // 关键: 括号比1粗(density>0.45),比3细(density<0.55)
    else if (aspectRatio < 0.35 && density > 0.45 && density < 0.58 && numHoles == 0 && h > 30) {
        // 通过左右像素分布判断是左括号还是右括号
        // 左括号(: 左侧像素多(弯向右侧); 右括号): 右侧像素多(弯向左侧)
        int leftPixels = 0, rightPixels = 0;
        int halfW = w / 2;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < halfW; x++) {
                if (roi.at<uchar>(y, x) == 255) leftPixels++;
            }
            for (int x = halfW; x < w; x++) {
                if (roi.at<uchar>(y, x) == 255) rightPixels++;
            }
        }
        if (leftPixels > rightPixels) {
            result = '(';
        } else {
            result = ')';
        }
    }
    // 减号: 单条横线,高度很小,宽度很长,密度高(实心)
    else if (h <= 10 && aspectRatio > 2.0 && density > 0.8) {
        result = '-';
    }
    // 等号: 两条横线合并后的,或者单条但位置偏下
    else if (h < 30 && h > 10 && aspectRatio > 1.5 && density > 0.4 && numHoles <= 1) {
        result = '=';
    }
    // 加号: 正方形,密度低(中间有空洞但不算作hole)
    else if (aspectRatio > 0.85 && aspectRatio < 1.15 && density > 0.2 && density < 0.35) {
        result = '+';
    }
    // 乘号: 正方形或略宽,密度中等
    else if (aspectRatio > 0.8 && aspectRatio < 1.2 && density > 0.4 && density < 0.65 && numHoles == 0 && h < 28) {
        result = 'x';
    }
    // 数字识别 - 按优先级从高到低,精确特征匹配
    // 1: 细长,低密度,无孔洞
    else if (aspectRatio < 0.65 && density < 0.43 && numHoles == 0) {
        result = '1';
    }
    // 1 (宽一点的字体): 细长,中等密度,无孔洞,中部最轻
    else if (aspectRatio >= 0.65 && aspectRatio < 0.75 && density >= 0.43 && density < 0.55 &&
             numHoles == 0 && midRatio < 0.25) {
        result = '1';
    }
    // 8: 两个孔洞,密度高 - 必须优先判断,避免被误判为除号
    else if (numHoles >= 2 && density > 0.55 && aspectRatio > 0.6 && aspectRatio <= 0.85 && h >= 25) {
        result = '8';
    }
    // 除号: 合并后的整体(点-线-点结构),有2个孔(上下点),低密度
    else if (numHoles >= 2 && h > 15 && h < 35 && density < 0.40 && aspectRatio > 0.8 && aspectRatio < 1.6) {
        result = '/';
    }
    // 0: 一个孔洞,密度中等,上下相对均匀,中部不太重
    else if (numHoles >= 1 && density > 0.48 && density < 0.60 && midRatio < 0.30) {
        result = '0';
    }
    // 9: 一个孔洞,底部最轻(9的关键特征: 上重下轻)
    else if (numHoles >= 1 && density > 0.52 && botRatio < 0.32) {
        result = '9';
    }
    // 6: 一个孔洞,底部不是最轻,且中部>=顶部
    else if (numHoles >= 1 && density > 0.52 && midRatio >= topRatio) {
        result = '6';
    }
    // 4: 一个孔洞,密度较低,中部较重
    else if (numHoles >= 1 && density < 0.60 && midRatio > 0.28) {
        result = '4';
    }
    // 0 (兜底): 一个孔洞,其他情况
    else if (numHoles >= 1 && density > 0.48) {
        // 精细判断: 如果顶部明显>底部,可能是9
        if (topRatio > botRatio + 0.05 && botRatio < 0.35) {
            result = '9';
        } else if (midRatio > topRatio + 0.03 && midRatio > botRatio + 0.03) {
            result = '6';
        } else {
            result = '0';
        }
    }
    // 2: 无孔洞,顶部和底部较重,中部明显较轻
    else if (numHoles == 0 && topRatio > 0.30 && botRatio > 0.35 && midRatio < 0.28 && density > 0.45) {
        result = '2';
    }
    // 7: 顶部非常重,中下轻,密度低
    else if (numHoles == 0 && topRatio > 0.45 && midRatio < 0.28 && density < 0.45) {
        result = '7';
    }
    // 3: 无孔洞,底部较重,中部较轻
    else if (numHoles == 0 && density > 0.45 && botRatio > 0.35 && midRatio < 0.32) {
        result = '3';
    }
    // 5: 无孔洞,中部相对较重,上中下较均匀
    else if (numHoles == 0 && density > 0.47 && midRatio > 0.32 && topRatio > 0.28) {
        result = '5';
    }
    // 兜底判断 - 无孔洞数字的最终分类
    else if (numHoles == 0) {
        if (aspectRatio < 0.65 && density < 0.43) result = '1';
        else if (density < 0.48) result = '5';
        else if (midRatio < 0.25) result = '2';
        else result = '3';
    }

    return result;
}

vector<RecognizedChar> FormulaRecognizer::detectCharacters(const Mat& binary) {
    vector<RecognizedChar> characters;

    vector<vector<Point>> contours;
    findContours(binary.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    vector<Rect> boxes;
    for (const auto& contour : contours) {
        Rect box = boundingRect(contour);
        if (box.width < 3 || box.height < 3 || box.area() < 15) {
            continue;
        }
        boxes.push_back(box);
    }

    sort(boxes.begin(), boxes.end(),
         [](const Rect& a, const Rect& b) { return a.x < b.x; });

    // 合并等号的两条横线 和 除号(÷)的三个部分
    vector<Rect> mergedBoxes;
    size_t i = 0;
    while (i < boxes.size()) {
        Rect box = boxes[i];

        // 检测除号: 可能是 横线+上点+下点 或 上点+横线+下点 等顺序
        // 先找横线
        if (box.height < 8 && box.width > box.height * 2.5) {  // 横线
            // 检查前后是否有小点
            vector<size_t> dotIndices;
            for (size_t j = (i > 0 ? i - 1 : 0); j < min(i + 3, boxes.size()); j++) {
                if (j == i) continue;
                Rect testBox = boxes[j];
                if (testBox.height < 8 && testBox.width < 8 && testBox.area() < 35) {
                    int xDiff = abs(testBox.x - box.x);
                    int yDiff = abs(testBox.y - box.y);
                    if (xDiff < 20 && yDiff > 2 && yDiff < 20) {
                        dotIndices.push_back(j);
                    }
                }
            }

            // 如果找到2个点,合并为除号
            if (dotIndices.size() >= 2) {
                vector<Rect> allBoxes = {box};
                for (size_t idx : dotIndices) {
                    allBoxes.push_back(boxes[idx]);
                }

                int minX = box.x, minY = box.y, maxX = box.br().x, maxY = box.br().y;
                for (const auto& b : allBoxes) {
                    minX = min(minX, b.x);
                    minY = min(minY, b.y);
                    maxX = max(maxX, b.br().x);
                    maxY = max(maxY, b.br().y);
                }

                mergedBoxes.push_back(Rect(minX, minY, maxX - minX, maxY - minY));

                // 跳过已处理的框
                size_t maxIdx = *max_element(dotIndices.begin(), dotIndices.end());
                i = maxIdx + 1;
                continue;
            }
        }

        // 合并等号的两条横线
        if (box.height <= 10 && box.width > box.height * 3) {
            if (i + 1 < boxes.size()) {
                Rect nextBox = boxes[i + 1];
                int xDiff = abs(box.x - nextBox.x);
                int yDiff = abs(box.y - nextBox.y);

                if (nextBox.height <= 10 && nextBox.width > nextBox.height * 3 &&
                    xDiff < 10 && yDiff > 3 && yDiff < 20) {
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

    // 识别每个字符,直到遇到等号就停止
    for (const auto& box : mergedBoxes) {
        Mat roi = binary(box);
        char recognized = recognizeCharacter(roi, box);

        if (recognized != '?') {
            characters.push_back(RecognizedChar(recognized, box));

            // 遇到等号就停止识别后续字符
            if (recognized == '=') {
                break;
            }
        }
    }

    return characters;
}

double FormulaRecognizer::evaluateExpression(const string& expr) {
    string expression = expr;

    if (!expression.empty() && expression.back() == '=') {
        expression.pop_back();
    }

    for (size_t i = 0; i < expression.length(); i++) {
        if (expression[i] == 'x') expression[i] = '*';
    }

    // 使用双栈算法实现运算符优先级(支持括号)
    try {
        stack<double> numbers;
        stack<char> operators;

        // 运算符优先级判断
        auto precedence = [](char op) -> int {
            if (op == '(') return 0;  // 括号优先级最低
            if (op == '+' || op == '-') return 1;
            if (op == '*' || op == '/') return 2;
            return 0;
        };

        // 执行运算
        auto applyOp = [](double a, double b, char op) -> double {
            switch (op) {
                case '+': return a + b;
                case '-': return a - b;
                case '*': return a * b;
                case '/': return (b != 0) ? a / b : 0;
                default: return 0;
            }
        };

        size_t i = 0;
        while (i < expression.length()) {
            if (expression[i] == ' ') {
                i++;
                continue;
            }

            if (expression[i] == 's') {
                i++;
                double num = 0;
                while (i < expression.length() && isdigit(expression[i])) {
                    num = num * 10 + (expression[i] - '0');
                    i++;
                }
                numbers.push(sqrt(num));
            }
            else if (expression[i] == '(') {
                operators.push('(');
                i++;
            }
            else if (expression[i] == ')') {
                while (!operators.empty() && operators.top() != '(') {
                    if (numbers.size() < 2) return 0.0;

                    char op = operators.top();
                    operators.pop();

                    double b = numbers.top(); numbers.pop();
                    double a = numbers.top(); numbers.pop();

                    numbers.push(applyOp(a, b, op));
                }

                if (!operators.empty() && operators.top() == '(') {
                    operators.pop();
                }
                i++;
            }
            else if (isdigit(expression[i])) {
                double num = 0;
                while (i < expression.length() && isdigit(expression[i])) {
                    num = num * 10 + (expression[i] - '0');
                    i++;
                }
                numbers.push(num);
            }
            else if (expression[i] == '+' || expression[i] == '-' ||
                     expression[i] == '*' || expression[i] == '/') {
                char op = expression[i];

                while (!operators.empty() && operators.top() != '(' &&
                       precedence(operators.top()) >= precedence(op)) {
                    if (numbers.size() < 2) return 0.0;

                    char topOp = operators.top();
                    operators.pop();

                    double b = numbers.top(); numbers.pop();
                    double a = numbers.top(); numbers.pop();

                    numbers.push(applyOp(a, b, topOp));
                }

                operators.push(op);
                i++;
            }
            else {
                i++;
            }
        }

        while (!operators.empty()) {
            if (numbers.size() < 2) return 0.0;

            char op = operators.top();
            operators.pop();

            if (op == '(') continue;  // 忽略未匹配的左括号

            double b = numbers.top(); numbers.pop();
            double a = numbers.top(); numbers.pop();

            numbers.push(applyOp(a, b, op));
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

    string expression;
    for (const auto& ch : chars) {
        expression += ch.character;
        if (ch.character == '=') {
            equalsSignBox = ch.boundingBox;
        }
    }

    cout << "识别的字符序列: " << expression << endl;

    double result = evaluateExpression(expression);

    return make_pair(expression, result);
}

void FormulaRecognizer::writeResultToImage(const Mat& image, const string& formula,
                                          double result, const string& outputPath) {
    Mat outputImage = image.clone();

    int textX, textY;
    if (equalsSignBox.width > 0) {
        textX = equalsSignBox.x + equalsSignBox.width + 10;
        textY = equalsSignBox.y + equalsSignBox.height;
    } else {
        textX = image.cols - 150;
        textY = image.rows / 2;
    }

    stringstream ss;
    if (result == floor(result)) {
        ss << static_cast<int>(result);
    } else {
        ss << fixed << setprecision(2) << result;
    }
    string resultText = ss.str();

    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 1.5;
    int thickness = 3;
    Scalar textColor(0, 0, 255);

    int baseline = 0;
    Size textSize = getTextSize(resultText, fontFace, fontScale, thickness, &baseline);

    if (textX + textSize.width > image.cols) {
        textX = image.cols - textSize.width - 10;
    }
    if (textY > image.rows) {
        textY = image.rows - 10;
    }
    if (textY - textSize.height < 0) {
        textY = textSize.height + 10;
    }

    Point textOrg(textX, textY);
    rectangle(outputImage,
              Point(textX - 5, textY - textSize.height - 5),
              Point(textX + textSize.width + 5, textY + baseline + 5),
              Scalar(255, 255, 255),
              FILLED);

    putText(outputImage, resultText, textOrg, fontFace, fontScale,
            textColor, thickness, LINE_AA);

    imwrite(outputPath, outputImage);
}

// 检测多个公式行
vector<Rect> FormulaRecognizer::detectFormulaRows(const Mat& binary) {
    vector<Rect> rowRects;

    vector<int> horizontalProjection(binary.rows, 0);
    for (int y = 0; y < binary.rows; y++) {
        for (int x = 0; x < binary.cols; x++) {
            if (binary.at<uchar>(y, x) == 255) {
                horizontalProjection[y]++;
            }
        }
    }

    bool inFormula = false;
    int startY = 0;

    for (int y = 0; y < binary.rows; y++) {
        if (horizontalProjection[y] > 0) {
            if (!inFormula) {
                startY = y;
                inFormula = true;
            }
        } else {
            if (inFormula) {
                int endY = y - 1;
                if (endY - startY > 10) {
                    rowRects.push_back(Rect(0, startY, binary.cols, endY - startY + 1));
                }
                inFormula = false;
            }
        }
    }

    if (inFormula) {
        int endY = binary.rows - 1;
        if (endY - startY > 10) {
            rowRects.push_back(Rect(0, startY, binary.cols, endY - startY + 1));
        }
    }

    return rowRects;
}

vector<FormulaResult> FormulaRecognizer::recognizeMultipleFormulas(const Mat& image) {
    vector<FormulaResult> results;

    cout << "开始多公式识别..." << endl;

    Mat binary = preprocessImage(image);

    vector<Rect> formulaRows = detectFormulaRows(binary);

    if (formulaRows.empty()) {
        cout << "警告: 未检测到任何公式行!" << endl;
        return results;
    }

    cout << "检测到 " << formulaRows.size() << " 个公式行" << endl;

    for (size_t i = 0; i < formulaRows.size(); i++) {
        cout << "\n--- 识别第 " << (i + 1) << " 个公式 ---" << endl;

        Rect row = formulaRows[i];

        Mat rowImage = image(row);

        auto result_pair = recognizeFormula(rowImage);
        string expression = result_pair.first;
        double result = result_pair.second;

        cout << "识别的字符序列: " << expression << endl;
        cout << "计算结果: " << result << endl;

        Mat rowBinary = preprocessImage(rowImage);
        vector<RecognizedChar> chars = detectCharacters(rowBinary);

        Rect localEqualsBox;
        for (const auto& ch : chars) {
            if (ch.character == '=') {
                localEqualsBox = ch.boundingBox;
                break;
            }
        }

        FormulaResult formulaResult;
        formulaResult.expression = expression;
        formulaResult.result = result;
        formulaResult.boundingBox = row;
        formulaResult.equalsSignBox = Rect(
            localEqualsBox.x + row.x,
            localEqualsBox.y + row.y,
            localEqualsBox.width,
            localEqualsBox.height
        );

        results.push_back(formulaResult);
    }

    return results;
}

void FormulaRecognizer::writeMultipleResultsToImage(const Mat& image,
                                                   const vector<FormulaResult>& results,
                                                   const string& outputPath) {
    Mat outputImage = image.clone();

    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 1.5;
    int thickness = 3;
    Scalar textColor(0, 0, 255);

    for (const auto& formulaResult : results) {
        stringstream ss;
        if (formulaResult.result == floor(formulaResult.result)) {
            ss << static_cast<int>(formulaResult.result);
        } else {
            ss << fixed << setprecision(2) << formulaResult.result;
        }
        string resultText = ss.str();

        int textX = formulaResult.equalsSignBox.x + formulaResult.equalsSignBox.width + 10;
        int textY = formulaResult.equalsSignBox.y + formulaResult.equalsSignBox.height;

        int baseline = 0;
        Size textSize = getTextSize(resultText, fontFace, fontScale, thickness, &baseline);

        if (textX + textSize.width > image.cols) {
            textX = image.cols - textSize.width - 10;
        }
        if (textY > image.rows) {
            textY = image.rows - 10;
        }
        if (textY - textSize.height < 0) {
            textY = textSize.height + 10;
        }

        Point textOrg(textX, textY);
        rectangle(outputImage,
                  Point(textX - 5, textY - textSize.height - 5),
                  Point(textX + textSize.width + 5, textY + baseline + 5),
                  Scalar(255, 255, 255),
                  FILLED);

        putText(outputImage, resultText, textOrg, fontFace, fontScale,
                textColor, thickness, LINE_AA);
    }

    imwrite(outputPath, outputImage);

    cout << "所有结果已写入图片: " << outputPath << endl;
}
