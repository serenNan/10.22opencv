/**
 * 公式识别系统 - 头文件
 * 定义公式识别相关的类和结构体
 */

#ifndef FORMULA_RECOGNIZER_H
#define FORMULA_RECOGNIZER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <utility>

using namespace cv;
using namespace std;

// 识别的字符结构体
struct RecognizedChar {
    char character;
    Rect boundingBox;
    float confidence;

    RecognizedChar(char c, Rect box, float conf = 1.0);
    bool operator<(const RecognizedChar& other) const;
};

// 公式识别器类
class FormulaRecognizer {
private:
    bool debug;
    Rect equalsSignBox;  // 保存等号的位置

    // 私有方法
    Mat preprocessImage(const Mat& input);
    char recognizeCharacter(const Mat& roi, const Rect& box);
    vector<RecognizedChar> detectCharacters(const Mat& binary);
    double evaluateExpression(const string& expr);

public:
    FormulaRecognizer(bool enableDebug = false);
    pair<string, double> recognizeFormula(const Mat& image);

    // 在图片上写入结果并保存
    void writeResultToImage(const Mat& image, const string& formula, double result,
                           const string& outputPath);
};

#endif // FORMULA_RECOGNIZER_H
