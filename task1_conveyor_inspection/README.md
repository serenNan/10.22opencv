# 流水线产品质量检测系统

基于OpenCV的实时视频产品检测与质量分类系统。

## 功能特点

- ✅ **白色背景检测**：通用检测方法，不依赖产品颜色
- ✅ **形状分类**：基于填充度自动识别矩形（合格品）与其他形状（次品）
- ✅ **多方向追踪**：支持产品从左右、上下四个方向移动
- ✅ **实时可视化**：视频播放模式（WSL自动切换到文件输出）
- ✅ **准确计数**：精确统计合格品和次品数量

## 编译

```bash
mkdir build && cd build
cmake ..
make
```

## 使用方法

### 基本用法（仅统计）

```bash
./conveyor_inspection_cli ../video/1.mp4
```

### 视频可视化模式

```bash
./conveyor_inspection_cli ../video/1.mp4 --show
```

**注意**：在WSL环境下，由于GUI限制，程序会自动切换到视频文件输出模式，生成带检测结果的视频文件（如 `1_result.mp4`）。

### 保存调试帧

```bash
./conveyor_inspection_cli ../video/1.mp4 --save-frames
```

### 组合选项

```bash
./conveyor_inspection_cli ../video/1.mp4 --show --save-frames
```

## 输出说明

### 命令行输出

- 实时显示每帧的检测信息（顶点数、面积、填充度）
- 产品通过计数线时输出计数信息（ID、方向、类型）
- 最终统计报告（合格品、次品、总数）

### 视频输出（--show 模式）

生成的结果视频包含：

- **绿色框**：合格品（矩形）
- **红色框**：次品（其他形状）
- **青色线**：计数线（左侧20%，右侧80%）
- **ID标签**：每个产品的追踪ID和状态（✓/✗）
- **统计信息**：实时显示帧数、合格品数、次品数、总数

### 播放控制

在GUI窗口中（如果支持）：
- `ESC` 或 `q` - 退出播放

## 检测原理

### 1. 白色背景检测

```cpp
// 检测白色背景（HSV空间）
Scalar lower_white(0, 0, 200);     // H不限，S低，V高
Scalar upper_white(179, 30, 255);
inRange(hsv, lower_white, upper_white, mask);

// 反转掩码：背景→黑，产品→白
bitwise_not(mask, mask);
```

### 2. 形状分类

基于**填充度**判断产品类型：

```cpp
float fill_ratio = contour_area / bounding_rect_area;

if (fill_ratio > 0.78) {
    // 矩形（合格品）：填充度 > 0.78
} else {
    // 其他形状（次品）：填充度 < 0.78
}
```

### 3. 多方向追踪

- 记录产品初始位置
- 计算移动方向（dx, dy）
- 根据方向设置不同计数线：
  - 左→右：右侧计数线（80%）
  - 右→左：左侧计数线（20%）
  - 上→下 / 下→上：垂直计数线

## 检测结果

### 视频 1

- **合格品**：3 个
- **次品**：3 个
- **总计**：6 个

### 视频 2

- **合格品**：1 个
- **次品**：7 个
- **总计**：8 个

## 参数调节

如需调整检测参数，可使用 HSV 调节工具：

```bash
./hsv_tuner_headless ../video/2.mp4 debug_output
```

详见 [HSV_TUNER_README.md](HSV_TUNER_README.md)

## 技术栈

- **OpenCV 4.8.0**
- **C++11**
- **CMake 3.10+**

## 项目结构

```
.
├── main.cpp                    # 程序入口
├── conveyor_inspector.h/cpp    # 核心检测逻辑
├── hsv_tuner.cpp              # GUI参数调节工具
├── hsv_tuner_headless.cpp     # 无GUI参数调节工具
├── CMakeLists.txt             # 编译配置
├── video/                     # 测试视频
│   ├── 1.mp4
│   ├── 2.mp4
│   ├── 1_result.mp4          # 生成的结果视频
│   └── 2_result.mp4
└── build/                     # 编译输出
```

## 优势

1. **通用性**：不依赖产品颜色，适用于任何白色背景的检测场景
2. **鲁棒性**：基于填充度的形状判断，抗噪声能力强
3. **灵活性**：支持多方向产品移动，适应不同流水线布局
4. **可视化**：直观的视频输出，便于调试和验证

## 许可

MIT License
