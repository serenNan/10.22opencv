/**
 * 流水线产品质量检测系统 - 头文件
 * 定义产品检测和追踪相关的类和结构体
 */

#ifndef CONVEYOR_INSPECTOR_H
#define CONVEYOR_INSPECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

using namespace cv;
using namespace std;

// 产品追踪结构体
struct TrackedProduct {
    int id;                // 产品唯一ID
    Point2f centroid;      // 质心坐标
    Point2f initial_pos;   // 初始位置(用于判断移动方向)
    int frames_tracked;    // 已追踪帧数
    int frames_lost;       // 丢失帧数计数
    bool counted;          // 是否已统计
};

// 检测结果结构体
struct Detection {
    string type;           // "qualified"(合格) 或 "defective"(次品)
    Point2f centroid;      // 质心坐标
    float angle;           // 旋转角度
    float scale;           // 缩放倍数
    RotatedRect rect;      // 最小外接矩形
    vector<Point> box;     // 边界框顶点
};

// 产品追踪器类
class ProductTracker {
private:
    vector<TrackedProduct> tracked_products;
    int next_id;
    float distance_threshold;

public:
    ProductTracker(float dist_thresh = 80.0f);
    vector<TrackedProduct>& update(const vector<Point2f>& centroids);
};

// 流水线检测器类
class ConveyorInspector {
private:
    bool save_frames;
    int frame_count;
    int qualified_count;
    int defective_count;
    ProductTracker tracker;
    int counting_line_x;
    const float REFERENCE_SIZE = 200.0f;

    // 私有方法
    vector<Detection> detectProducts(const Mat& frame);
    void updateCounts(const vector<Detection>& detections,
                     vector<TrackedProduct>& tracked);

public:
    ConveyorInspector(bool save_frames = false);
    void processVideo(const string& video_path);
    void printStatistics(const string& video_path);
};

#endif // CONVEYOR_INSPECTOR_H
