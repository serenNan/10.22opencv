# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a computer vision project repository for implementing two vision-based inspection and recognition systems. The project requirements are documented in Chinese in the `要求/` directory.

### Vision Task 1: Conveyor Belt Product Quality Inspection (流水线产品识别)

**Goal**: Analyze video from a simulated conveyor belt to detect and count defective vs. qualified products as they pass from right to left across the screen.

**Basic Requirements**:
- Read and parse the provided simulated video files (MP4 format in `video/` directory)
- When video playback ends, output a statistical report showing counts of defective products and qualified products
- Display the report in the console or GUI window

**Extended Requirements**:
- Handle rotation and scale variations in products (shown as "缩放" - scaling in images)
- Output rotation angles and scale factors when detected
- Real-time output of qualified/defective counts as products pass through the detection belt
- Fast recognition capability
- Allow creative extensions related to the project theme

### Vision Task 2: Formula Recognition (公式识别)

**Goal**: Perform OCR on calculator-style mathematical expressions from images.

**Basic Requirements**:
- Recognize expressions containing Arabic numerals (0-9) and basic operators (+, -, ×, ÷, =)
- Output recognized formula content and computed results in structured format

**Extended Requirements**:
- Recognize multiple formulas in a single image
- Recognize complex nested formulas
- Handle rotated formulas (may contain local rotation, but symbols within formulas should not be rotated)
- Recognize compound operators (e.g., checkmark/radical, exponent, angle symbols)

## Project Structure

```
.
├── video/          # Sample video files for Task 1 (conveyor belt inspection)
│   ├── 1.mp4
│   └── 2.mp4
├── 要求/           # Requirements documentation (images showing project specs)
│   ├── 1.jpg      # Task specifications page 1
│   └── 2.jpg      # Task specifications page 2
└── .gitignore     # Git ignore rules (excludes .vscode and .history)
```

## Development Approach

### For Task 1 (Conveyor Belt Inspection):
- Use OpenCV for video processing and object detection
- Consider classical CV approaches (contour detection, template matching, feature matching) or deep learning (YOLO, SSD, etc.)
- Implement rotation-invariant and scale-invariant detection
- Process video frame-by-frame, tracking objects as they move left across the screen
- Maintain counters for defective vs. qualified products

### For Task 2 (Formula Recognition):
- Use OCR libraries (Tesseract, PaddleOCR, EasyOCR) or custom trained models
- Preprocess images: binarization, deskewing, noise removal
- Segment individual characters and operators
- Parse spatial relationships to reconstruct formula structure
- Implement formula evaluation logic for basic arithmetic

## Technology Stack Considerations

This is a computer vision project that will likely use:
- **Python** with OpenCV as the primary framework
- **OCR engines** for formula recognition (Tesseract, PaddleOCR, or deep learning models)
- **Deep learning frameworks** (PyTorch/TensorFlow) if using neural networks for detection/classification
- **NumPy/SciPy** for numerical operations

## Key Implementation Notes

1. **Video files are in the `video/` directory** - use these for Task 1 development and testing
2. **Output requirements**: Both tasks require clear statistical output (counts, recognized content, computed results)
3. **Performance matters**: Task 1 specifically mentions "fast recognition" as a requirement
4. **Robustness**: Handle rotation, scaling, and varying image quality
5. **Chinese context**: Requirements documentation is in Chinese, but code should use English identifiers
