/**
 * 流水线产品质量检测系统 - 主程序
 * 命令行入口,处理参数并调用检测器
 */

#include "conveyor_inspector.h"
#include <iostream>

using namespace std;

void printUsage(const char* program_name) {
    cout << "流水线产品质量检测系统 v1.0" << endl;
    cout << endl;
    cout << "用法: " << program_name << " <视频路径> [选项]" << endl;
    cout << endl;
    cout << "选项:" << endl;
    cout << "  --save-frames    保存调试帧到输出目录" << endl;
    cout << endl;
    cout << "示例:" << endl;
    cout << "  " << program_name << " video/1.mp4" << endl;
    cout << "  " << program_name << " video/1.mp4 --save-frames" << endl;
    cout << endl;
}

int main(int argc, char** argv) {
    // 检查参数
    if (argc < 2) {
        printUsage(argv[0]);
        return -1;
    }

    string video_path = argv[1];
    bool save_frames = false;

    // 解析选项
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--save-frames") {
            save_frames = true;
        }
    }

    // 创建检测器并处理视频
    ConveyorInspector inspector(save_frames);
    inspector.processVideo(video_path);
    inspector.printStatistics(video_path);

    return 0;
}
