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

// 单个公式结果结构体
struct FormulaResult {
    string expression;      // 识别的公式表达式
    double result;          // 计算结果
    Rect boundingBox;       // 公式在图片中的位置
    Rect equalsSignBox;     // 等号的位置
};


// 公式识别器类
class FormulaRecognizer {
private:
    Rect equalsSignBox;  // 保存等号的位置

    // 私有方法
    Mat preprocessImage(const Mat& input);
    char recognizeCharacter(const Mat& roi, const Rect& box);
    vector<RecognizedChar> detectCharacters(const Mat& binary);
    double evaluateExpression(const string& expr);
    vector<Rect> detectFormulaRows(const Mat& binary);

public:
    FormulaRecognizer();
    pair<string, double> recognizeFormula(const Mat& image);
    vector<FormulaResult> recognizeMultipleFormulas(const Mat& image);

    // 在图片上写入结果并保存
    void writeResultToImage(const Mat& image, const string& formula, double result,
                           const string& outputPath);
    void writeMultipleResultsToImage(const Mat& image, const vector<FormulaResult>& results,
                                    const string& outputPath);
};

#endif // FORMULA_RECOGNIZER_H
