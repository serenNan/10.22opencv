# 计算机视觉项目集合

基于OpenCV的计算机视觉系统,包含产品质量检测和公式识别两个独立任务。

## 项目概述

本系统实现了两个独立的计算机视觉任务:
1. **任务1: 流水线产品识别** ✅ - 检测视频中从右向左移动的PCB板,分类为合格品(矩形)或次品(三角形)
2. **任务2: 公式识别** ✅ - 识别图片中的数学公式并计算结果

## 项目结构

```
.
├── CMakeLists.txt                           # 根CMake配置(构建所有任务)
├── task1_conveyor_inspection/               # 任务1: 流水线产品检测
│   ├── CMakeLists.txt                       # 任务1独立构建配置
│   ├── conveyor_inspection_cli.cpp          # 源代码
│   └── video/                               # 测试视频
│       ├── 1.mp4
│       └── 2.mp4
├── task2_formula_recognition/               # 任务2: 公式识别
│   ├── CMakeLists.txt                       # 任务2独立构建配置
│   ├── formula_recognition.cpp              # 源代码
│   └── formula_images/                      # 测试图片
│       ├── formula_1.png ... formula_6.png
├── build/                                   # 编译输出目录
│   ├── task1_conveyor_inspection/
│   │   └── conveyor_inspection_cli          # 任务1可执行文件
│   └── task2_formula_recognition/
│       └── formula_recognition_cli          # 任务2可执行文件
└── README.md                                # 本文件
```

## 编译方法

### 方法1: 一次性构建所有任务(推荐)

```bash
# 从项目根目录
mkdir build
cd build
cmake ..
make

# 两个可执行文件位于:
# build/task1_conveyor_inspection/conveyor_inspection_cli
# build/task2_formula_recognition/formula_recognition_cli
```

### 方法2: 单独构建某个任务

#### 构建任务1
```bash
cd task1_conveyor_inspection
mkdir build
cd build
cmake ..
make

# 可执行文件: ./conveyor_inspection_cli
```

#### 构建任务2
```bash
cd task2_formula_recognition
mkdir build
cd build
cmake ..
make

# 可执行文件: ./formula_recognition_cli
```

## 依赖要求

- OpenCV 4.x
- C++11或更高版本
- CMake 3.10或更高版本

---

# 任务1: 流水线产品识别

## 功能特点

- 实时检测和分类PCB产品(合格品/次品)
- 自动跟踪产品避免重复计数
- 检测产品的旋转角度和缩放比例
- 实时输出检测结果
- 最终统计报告

## 使用方法

```bash
# 从build目录运行
cd build/task1_conveyor_inspection

# 基本模式
./conveyor_inspection_cli ../../video/1.mp4

# 保存调试帧
./conveyor_inspection_cli ../../video/1.mp4 --save-frames
```

## 检测结果

### 视频1 (video/1.mp4)
- 合格品: 3个
- 次品: 3个
- 总计: 6个

### 视频2 (video/2.mp4)
- 合格品: 4个
- 次品: 2个
- 总计: 6个

## 技术实现

### 检测算法

1. **颜色检测**
   - 使用HSV颜色空间检测绿色PCB板
   - 颜色范围: H(35-85), S(40-255), V(40-255)

2. **形状分类**
   - 矩形PCB → 合格品
   - 三角形PCB → 次品
   - 使用轮廓近似和凸包分析

3. **产品跟踪**
   - 基于质心的距离跟踪
   - 防止重复计数
   - 设置计数线(左侧20%位置)

4. **旋转和缩放检测**
   - 使用minAreaRect获取旋转矩形
   - 计算角度和缩放因子
   - 参考尺寸: 200像素

## 输出示例

```
============================================================
Processing: ../../video/1.mp4
============================================================

Frame 82: DEFECTIVE detected - Angle: 90.0°, Scale: 1.09x | Qualified: 0, Defective: 1
Frame 99: QUALIFIED detected - Angle: 90.0°, Scale: 1.34x | Qualified: 1, Defective: 1
...

============================================================
最终统计报告
============================================================
视频: ../../video/1.mp4
------------------------------------------------------------
合格品数量: 3
次品数量:   3
总计:       6
============================================================
```

---

# 任务2: 公式识别

## 功能说明

基于OpenCV的数学公式识别系统,可以从图片中识别数学表达式并计算结果。

### 支持的功能

**基础任务**:
- ✅ 识别阿拉伯数字 (0-9)
- ✅ 识别基本运算符 (+, -, ×, ÷, =)
- ✅ 输出识别的公式内容
- ✅ 计算并输出结果

**扩展任务**:
- ⚠️ 识别多个公式 (部分支持)
- ⚠️ 识别旋转的公式 (算法已实现,精度有限)
- ❌ 识别复杂嵌套公式 (需深度学习模型)
- ❌ 识别复合运算符 (需专业OCR引擎)

## 使用方法

```bash
# 从build目录运行
cd build/task2_formula_recognition

# 基本模式
./formula_recognition_cli ../../formula_images/formula_1.png

# 调试模式(显示每个字符的特征值)
./formula_recognition_cli ../../formula_images/formula_1.png --debug
```

## 测试图片

项目提供了6张测试图片在 `task2_formula_recognition/formula_images/` 目录:
- formula_1.png: 12+34=46
- formula_2.png: 56-23=33
- formula_3.png: 8×9=72
- formula_4.png: 100÷5=20
- formula_5.png: 3+5×2=13
- formula_6.png: 45-12+8=41

## 技术实现

### 识别流程

1. **图像预处理**
   - 灰度化转换
   - 自适应阈值二值化
   - 形态学去噪

2. **字符分割**
   - 轮廓检测
   - 边界框提取
   - 等号横线合并
   - 按x坐标排序

3. **字符识别**
   - 基于形态学特征的识别算法
   - 计算像素密度、宽高比、轮廓特征
   - 使用启发式规则进行字符分类

4. **公式解析和计算**
   - 运算符替换
   - 表达式求值
   - 结果验证

## 已知限制

### 当前版本的限制

1. **识别精度问题**
   - 基于形态学特征的识别方法精度有限
   - 只适用于清晰的打印体数字和简单运算符
   - 手写体、模糊图片、复杂排版的识别效果差
   - 不同字体可能导致识别错误

2. **功能限制**
   - 不支持分数、根号、指数等复杂运算符
   - 不支持括号嵌套
   - 多公式识别依赖图像分割质量
   - 表达式计算采用简化算法(左到右计算,不考虑运算符优先级)

### 改进建议

为了获得更好的公式识别效果,建议:

1. **使用深度学习模型**
   - 训练CNN进行字符识别
   - 使用LSTM/Transformer进行序列识别
   - 参考: CRNN, Attention-based OCR模型

2. **集成专业OCR引擎**
   - 使用Tesseract C++ API
   - 或使用OpenCV DNN模块加载预训练模型
   - 参考: EAST文本检测 + CRNN识别

3. **改进特征提取**
   - 使用HOG、SIFT等高级特征
   - 增加模板匹配库
   - 实现字符级别的深度特征学习

## 输出示例

```
========================================
  公式识别系统 v1.0 (C++ CLI版本)
========================================

图像尺寸: 400 x 100

开始图像预处理...
正在检测字符...
检测到 8 个字符
识别的字符序列: 12+34=46

========================================
           识别结果
========================================
识别的表达式: 12+34=46
计算结果: 12+34 = 46
预期结果: 46
✓ 结果正确!
========================================
```

注意: 由于采用基于规则的识别方法,识别精度受图像质量和字体影响较大。

---

## 快速开始

```bash
# 1. 克隆或下载项目
cd /path/to/project

# 2. 创建build目录并编译
mkdir build && cd build
cmake .. && make

# 3. 运行任务1
cd task1_conveyor_inspection
./conveyor_inspection_cli ../../video/1.mp4

# 4. 运行任务2
cd ../task2_formula_recognition
./formula_recognition_cli ../../formula_images/formula_1.png
```

## 许可证

本项目仅用于教学和学习目的。
