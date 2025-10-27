# OpenCV 计算机视觉项目

基于 OpenCV 的计算机视觉实践项目，包含流水线产品质量检测和数学公式识别两个子任务。

## 项目概述

本项目包含两个独立的计算机视觉应用：

### 📦 [Task 1 - 流水线产品质量检测系统](task1_conveyor_inspection/README.md)
- 实时视频流产品质量检测与追踪
- 自动识别合格品（矩形 PCB）和次品（三角形异物）
- 旋转角度检测、缩放倍数计算
- 支持播放控制（暂停、加速）
- **测试结果**：Video 1 和 2 准确率均为 100%

### 📐 [Task 2 - 公式识别系统](task2_formula_recognition/README.md)
- 数学公式 OCR 识别（无需外部 OCR 库）
- 基于形态学特征的字符识别
- 支持数字、运算符、括号、根号
- 双栈算法实现表达式计算（支持运算符优先级）
- **测试结果**：8 个测试公式准确率 100%

## 环境配置

### 系统要求

- **操作系统**：Linux (测试环境: Ubuntu 22.04 / WSL2)
- **编译器**：GCC 7.0+ 或 Clang 5.0+（支持 C++11）
- **构建工具**：CMake 3.10+

### 依赖项

#### 核心依赖
```bash
# OpenCV 4.x（必需）
sudo apt update
sudo apt install -y libopencv-dev

# CMake 构建工具
sudo apt install -y cmake build-essential

# 可选：pkg-config（用于检测 OpenCV 版本）
sudo apt install -y pkg-config
```

#### 验证安装
```bash
# 检查 OpenCV 版本
pkg-config --modversion opencv4

# 检查 CMake 版本
cmake --version

# 检查 GCC 版本
g++ --version
```

**推荐版本**：
- OpenCV: 4.2.0+（项目测试版本: 4.6.0）
- CMake: 3.10+
- GCC: 7.0+

### 从源码编译 OpenCV（可选）

如果系统 OpenCV 版本过低或需要自定义编译：

```bash
# 1. 安装依赖
sudo apt install -y build-essential cmake git pkg-config
sudo apt install -y libgtk-3-dev libavcodec-dev libavformat-dev libswscale-dev
sudo apt install -y libv4l-dev libxvidcore-dev libx264-dev
sudo apt install -y libjpeg-dev libpng-dev libtiff-dev
sudo apt install -y libatlas-base-dev gfortran
sudo apt install -y python3-dev python3-numpy

# 2. 下载 OpenCV
cd ~/Tools
git clone https://github.com/opencv/opencv.git
cd opencv
git checkout 4.6.0  # 或其他稳定版本

# 3. 编译安装
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE \
      -D CMAKE_INSTALL_PREFIX=/usr/local \
      -D WITH_GTK=ON \
      -D WITH_V4L=ON \
      -D WITH_OPENGL=ON \
      -D BUILD_EXAMPLES=OFF \
      ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

## 快速开始

### 1. 克隆项目

```bash
git clone <repository-url>
cd 10.22opencv
```

### 2. 编译项目

```bash
# 创建 build 目录
mkdir -p build && cd build

# 配置 CMake
cmake ..

# 编译（使用 4 个并行任务）
make -j4
```

**编译输出**：
```
build/
├── task1_conveyor_inspection/
│   └── conveyor_inspection_cli    # 流水线检测可执行文件
└── task2_formula_recognition/
    └── formula_recognition_cli     # 公式识别可执行文件
```

### 3. 运行测试

#### Task 1 - 流水线检测
```bash
# 实时播放视频
./build/task1_conveyor_inspection/conveyor_inspection_cli video/1.mp4

# 仅统计模式（无 GUI）
./build/task1_conveyor_inspection/conveyor_inspection_cli video/1.mp4 --no-show
```

#### Task 2 - 公式识别
```bash
# 识别单个公式
./build/task2_formula_recognition/formula_recognition_cli formula_images/formula_1.png

# 强制单公式模式
./build/task2_formula_recognition/formula_recognition_cli formula_images/formula_1.png --single

# 自定义输出路径
./build/task2_formula_recognition/formula_recognition_cli formula_images/formula_1.png --output result.png
```

## 项目结构

```
10.22opencv/
├── README.md                           # 本文档
├── CMakeLists.txt                      # 根 CMake 配置
├── CLAUDE.md                           # Claude Code 项目指令
│
├── task1_conveyor_inspection/          # 任务 1：流水线检测
│   ├── README.md                       # 详细文档
│   ├── main.cpp                        # 命令行入口
│   ├── conveyor_inspector.h            # 类定义
│   ├── conveyor_inspector.cpp          # 核心实现
│   └── CMakeLists.txt                  # 子项目配置
│
├── task2_formula_recognition/          # 任务 2：公式识别
│   ├── README.md                       # 详细文档
│   ├── main.cpp                        # 命令行入口
│   ├── formula_recognizer.h            # 类定义
│   ├── formula_recognizer.cpp          # 核心实现
│   ├── CLAUDE.md                       # 子项目指令
│   └── CMakeLists.txt                  # 子项目配置
│
├── video/                              # 测试视频
│   ├── 1.mp4                           # 流水线测试视频 1
│   └── 2.mp4                           # 流水线测试视频 2
│
├── formula_images/                     # 测试图片
│   ├── formula_1.png                   # 加法公式
│   ├── formula_2.png                   # 减法公式
│   ├── formula_3.png                   # 乘法公式
│   ├── formula_4.png                   # 除法公式
│   ├── formula_5.png                   # 混合运算
│   ├── formula_6.png                   # 多步运算
│   ├── formula_7.png                   # 括号运算
│   └── formula_8.png                   # 根号运算
│
├── build/                              # 编译输出目录（自动生成）
│   ├── task1_conveyor_inspection/
│   │   └── conveyor_inspection_cli
│   └── task2_formula_recognition/
│       └── formula_recognition_cli
│
└── 要求/                               # 项目需求文档（图片）
    ├── 1.jpg
    └── 2.jpg
```

## 详细文档

每个子项目都有独立的详细文档：

- **[Task 1 - 流水线产品质量检测系统](task1_conveyor_inspection/README.md)**
  - 技术实现细节（HSV 颜色分离、形态学处理、质心追踪）
  - 旋转角度和缩放倍数计算方法
  - 播放控制使用说明
  - 核心数据结构和算法

- **[Task 2 - 公式识别系统](task2_formula_recognition/README.md)**
  - 字符识别原理（形态学特征分析）
  - 表达式计算算法（双栈实现）
  - 字符分类规则表
  - 开发指南（如何添加新字符）

## 常见问题

### Q: 编译时找不到 OpenCV

**错误信息**：
```
CMake Error: Could not find OpenCV
```

**解决方案**：
```bash
# 方法 1: 安装 OpenCV 开发包
sudo apt install libopencv-dev

# 方法 2: 手动指定 OpenCV 路径
cmake -DOpenCV_DIR=/usr/local/lib/cmake/opencv4 ..

# 方法 3: 设置环境变量
export OpenCV_DIR=/usr/local/lib/cmake/opencv4
cmake ..
```

### Q: 编译时 C++ 标准不支持

**错误信息**：
```
error: 'auto' type specifier is a C++11 extension
```

**解决方案**：
确保 CMakeLists.txt 中设置了 C++11：
```cmake
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Q: 视频窗口无法显示（WSL2）

**问题描述**：在 WSL2 环境下无法显示视频窗口

**解决方案**：
```bash
# 使用 --no-show 参数运行（仅统计模式）
./build/task1_conveyor_inspection/conveyor_inspection_cli video/1.mp4 --no-show
```

### Q: 公式识别准确率低

**解决方案**：
1. 确保图片清晰，字符完整
2. 检查图片是否包含噪声或旋转
3. 查看详细文档了解支持的字符集
4. 参考 [Task 2 README](task2_formula_recognition/README.md) 中的字符识别规则

## 测试结果

### Task 1 - 流水线检测
- **Video 1**：合格品 3 个，次品 3 个 ✅ (准确率 100%)
- **Video 2**：合格品 4 个，次品 2 个 ✅ (准确率 100%)

### Task 2 - 公式识别
- **测试公式**：8 个
- **识别准确率**：100% (8/8) ✅
- **字符识别**：100% (53/53) ✅

## 技术栈

- **编程语言**：C++11
- **计算机视觉**：OpenCV 4.x
- **构建系统**：CMake 3.10+
- **识别方法**：经典 CV（无机器学习/深度学习）

## 贡献指南

本项目为教学演示项目，欢迎提交改进建议：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 许可证

本项目仅供学习和教学使用。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交 Issue
- 发送邮件

---

**项目开发环境**：Ubuntu 22.04 LTS / WSL2 + OpenCV 4.6.0 + CMake 3.22.1
