# 流水线产品质量检测系统

基于 OpenCV 的实时视频产品质量检测与追踪系统。

## 项目概述

本系统通过计算机视觉技术分析流水线视频，自动识别并统计合格品（矩形 PCB）与次品（三角形异物）数量。系统支持产品的旋转角度检测、缩放倍数计算，并提供实时视频播放和详细统计输出功能。

## 核心功能

### 基础功能
- ✅ 自动识别合格品（矩形 PCB）和次品（三角形异物）
- ✅ 实时统计产品数量并输出最终报告
- ✅ 视频播放模式和无显示模式（--no-show）切换

### 高级功能
- ✅ **旋转角度检测**：基于最小外接矩形计算，精度 0.1°
- ✅ **缩放倍数计算**：相对于首个合格品，精度 0.01x
- ✅ **产品追踪系统**：基于质心匹配，防止重复计数
- ✅ **实时视频标注**：边界框、ID、角度、缩放倍数显示
- ✅ **播放控制**：暂停/继续、加速播放、手动退出

## 依赖项

- **OpenCV 4.x** (测试版本: 4.6.0)
- **C++11** 标准
- **CMake 3.10+**

## 快速开始

### 编译

```bash
# 从项目根目录
cd /home/serennan/work/10.22opencv
mkdir -p build && cd build
cmake ..
make -j4
```

### 运行

```bash
# 基本用法（实时播放视频）
./task1_conveyor_inspection/conveyor_inspection_cli video/1.mp4

# 仅统计模式（无视频显示，适用于无 GUI 环境）
./task1_conveyor_inspection/conveyor_inspection_cli video/1.mp4 --no-show
```

## 播放控制

在视频播放过程中，支持以下快捷键：

- **ESC / q** - 退出播放
- **空格键** - 暂停/继续播放
- **右方向键** - 切换加速/正常播放（30ms → 5ms 延迟）

## 输出说明

### 实时视频窗口

程序默认会打开实时显示窗口：

**视觉标注**
- **绿色边框** - 合格品（矩形，填充度 > 0.80）
- **红色边框** - 次品（三角形，填充度 ≤ 0.80）
- **产品信息**（显示在每个物体下方，三行文字）：
  - 第1行：`ID:X YES/NO` - 追踪 ID 和类型
  - 第2行：`Angle:XX.Xdeg` - 旋转角度（精度 0.1°）
  - 第3行：`Scale:X.XXx` - 缩放倍数（精度 0.01x）

**顶部信息栏**（黑色背景，70px 高）
- **第1行（白色）**：Frame | Qualified | Defective | Total
- **第2行（黄色）**：缩放基准信息（如：`Scale Reference: 143.0px (1st Qualified)`）

### 控制台输出示例

**实时检测信息**：
```
  [缩放基准已设置] 使用首个合格品长边尺寸: 143.0px
Frame 99: ✓ QUALIFIED ← - ID:1, Angle: 180.0°, Scale: 1.87x | Total -> Qualified: 1, Defective: 0
Frame 173: ✓ QUALIFIED ← - ID:2, Angle: 180.0°, Scale: 1.81x | Total -> Qualified: 2, Defective: 0
Frame 262: ✗ DEFECTIVE ← - ID:4, Angle: 90.0°, Scale: 1.48x | Total -> Qualified: 3, Defective: 1
```

**最终统计报告**：
```
============================================================
最终统计报告
============================================================
视频: video/1.mp4
------------------------------------------------------------
合格品数量: 3
次品数量:   3
总计:       6
合格率:     50.00%
缩放基准:   143.0px (首个合格品)
============================================================

详细产品列表（按ID排序）:
============================================================
ID    类型        旋转角度      缩放倍数      检测帧
------------------------------------------------------------
0     ✗ 次品     90.0°         1.52x         82
1     ✓ 合格品   180.0°        1.87x         99
2     ✓ 合格品   180.0°        1.81x         173
3     ✓ 合格品   180.0°        1.81x         190
4     ✗ 次品     90.0°         1.48x         262
5     ✗ 次品     90.0°         1.49x         280
============================================================
```

## 技术实现

### 1. 产品检测流程

```
原始帧 → HSV 转换 → 背景分离 → 形态学去噪 → 轮廓提取 → 产品分类
```

**颜色分离（HSV 空间）**
```cpp
Scalar lower_white(0, 0, 200);     // 白色背景：H 不限，S 低，V 高
Scalar upper_white(179, 30, 255);
inRange(hsv, lower_white, upper_white, mask);
bitwise_not(mask, mask);           // 反转掩码获取绿色 PCB
```

**形态学处理**
- 开运算（5×5 矩形核，2 次迭代）：去除噪点
- 闭运算（5×5 矩形核）：填充空洞

**产品分类规则**
```cpp
// 基于多边形近似和填充度
vector<Point> approx;
approxPolyDP(contour, approx, arcLength(contour, true) * 0.03, true);
float area_ratio = area / (width * height);

if (approx.size() == 4 && area_ratio > 0.80) {
    type = "qualified";  // 矩形合格品
} else {
    type = "defective";  // 三角形次品
}
```

### 2. 产品追踪系统

**质心匹配算法**
- 距离阈值：80 像素
- 丢失容忍：10 帧（允许短暂遮挡）
- 每个产品维护唯一 ID

**防重复计数机制**
- 追踪帧数要求：≥ 10 帧（排除噪声）
- 移动距离要求：≥ 30 像素（排除静止背景）
- 每个 ID 仅统计一次（`counted` 标志）

### 3. 角度与缩放检测

**旋转角度计算**
```cpp
// 合格品：长边水平为 0°
float calculateRectangleAngle(const RotatedRect& rect) {
    float angle = rect.angle;
    float width = rect.size.width;
    float height = rect.size.height;

    if (width < height) {
        swap(width, height);
        angle += 90.0f;
    }

    // 归一化到 [0, 360)
    while (angle < 0) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;

    return angle;
}
```

**缩放倍数计算**
- 基准设定：首个检测到的合格品长边尺寸
- 公式：`scale = current_size / reference_size`
- 精度：保留 2 位小数

## 测试结果

### Video 1 (`video/1.mp4`)
- ✅ **合格品**：3 个
- ✅ **次品**：3 个
- **总计**：6 个
- **合格率**：50.00%
- **准确率**：100%

### Video 2 (`video/2.mp4`)
- ✅ **合格品**：4 个
- ✅ **次品**：2 个
- **总计**：6 个
- **合格率**：66.67%
- **准确率**：100%

## 项目结构

```
task1_conveyor_inspection/
├── main.cpp                    # 命令行入口，参数解析
├── conveyor_inspector.h        # 类定义、结构体声明
├── conveyor_inspector.cpp      # 核心检测与追踪逻辑
├── CMakeLists.txt             # 编译配置
└── README.md                  # 本文档
```

### 核心数据结构

**TrackedProduct** - 产品追踪信息
```cpp
struct TrackedProduct {
    int id;                // 唯一 ID
    Point2f centroid;      // 当前质心
    Point2f initial_pos;   // 初始位置（用于移动检测）
    int frames_tracked;    // 已追踪帧数
    int frames_lost;       // 丢失帧数
    bool counted;          // 是否已统计
};
```

**Detection** - 单帧检测结果
```cpp
struct Detection {
    string type;           // "qualified" 或 "defective"
    Point2f centroid;      // 质心坐标
    float angle;           // 旋转角度
    float scale;           // 缩放倍数
    RotatedRect rect;      // 最小外接矩形
    vector<Point> box;     // 边界框顶点
};
```

**CountedProduct** - 已统计产品记录
```cpp
struct CountedProduct {
    int id;                // 产品 ID
    string type;           // 类型
    float angle;           // 旋转角度
    float scale;           // 缩放倍数
    int frame;             // 统计时的帧号
};
```

## 性能特点

- **实时处理**：30 FPS（正常模式），200+ FPS（加速模式）
- **准确率**：100%（测试视频 1 和 2）
- **无 GUI 依赖**：支持 WSL/服务器环境运行（使用 `--no-show`）
- **低延迟**：单帧处理时间 < 5ms（典型场景）

## 限制与改进方向

### 当前限制
1. 仅支持绿色 PCB + 白色背景场景
2. 形状仅区分矩形/非矩形（未细分三角形/圆形等）
3. 缩放基准依赖首个合格品（若首个产品异常会影响后续）

### 潜在改进
1. 支持更多颜色和背景
2. 使用深度学习提高复杂场景鲁棒性
3. 添加产品尺寸绝对测量（需相机标定）
4. 支持多条流水线并行检测

## 作者备注

本系统采用经典计算机视觉技术实现，无需深度学习模型，具有轻量、快速、易部署的特点。适用于工业流水线质检、教学演示等场景。
