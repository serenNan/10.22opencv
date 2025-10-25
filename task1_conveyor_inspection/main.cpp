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
    cout << "  --no-show        禁用视频播放窗口（仅统计）" << endl;
    cout << "  --save-frames    保存调试帧到输出目录" << endl;
    cout << endl;
    cout << "示例:" << endl;
    cout << "  " << program_name << " video/1.mp4                    # 实时播放（默认）" << endl;
    cout << "  " << program_name << " video/1.mp4 --no-show          # 仅统计" << endl;
    cout << "  " << program_name << " video/1.mp4 --save-frames      # 播放+保存帧" << endl;
    cout << endl;
    cout << "播放控制:" << endl;
    cout << "  ESC 或 q - 退出播放" << endl;
    cout << endl;
    cout << "注意: 需要设置环境变量（已自动配置）:" << endl;
    cout << "  export QT_QPA_PLATFORM=wayland" << endl;
    cout << "  export WAYLAND_DISPLAY=wayland-0" << endl;
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
    bool show_video = true;  // 默认启用显示

    // 解析选项
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--save-frames") {
            save_frames = true;
        } else if (arg == "--no-show") {
            show_video = false;  // 使用 --no-show 禁用显示
        }
    }

    // 创建检测器并处理视频
    ConveyorInspector inspector(save_frames);
    inspector.processVideo(video_path, show_video);
    inspector.printStatistics(video_path);

    return 0;
}
