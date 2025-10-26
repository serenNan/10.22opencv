# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **formula recognition system** (公式识别系统) that performs OCR on calculator-style mathematical expressions from images using classical computer vision techniques. The system is implemented in C++ using OpenCV and uses morphological feature analysis for character recognition **without external OCR dependencies** (no Tesseract/PaddleOCR).

This is **Task 2** of a larger computer vision project (Task 1 is conveyor belt inspection in the parent directory).

## Build and Run

### Build Commands
```bash
# From project root
mkdir build && cd build
cmake ..
make

# Rebuild after changes
cd build && make
```

### Running the Program
```bash
# Basic usage
./build/formula_recognition_cli formula_images/formula_1.png

# With debug output (shows detailed recognition steps and intermediate images)
./build/formula_recognition_cli formula_images/formula_1.png --debug

# Debug mode generates:
# - debug_binary.png: Binary image after preprocessing
# - Detailed console output with character features
```

### Test Images
Six test formula images are available in `formula_images/`:
- `formula_1.png` through `formula_6.png`
- Each contains simple arithmetic expressions (e.g., "123+456=")

## Code Architecture

### Three-File Structure

1. **main.cpp** - CLI entry point
   - Argument parsing (image path, --debug flag)
   - Creates FormulaRecognizer instance
   - Displays final results

2. **formula_recognizer.h** - Interface definitions
   - `RecognizedChar` struct: character, bounding box, confidence
   - `FormulaRecognizer` class: main recognition pipeline

3. **formula_recognizer.cpp** - Core recognition logic (308 lines)
   - `preprocessImage()`: Grayscale → Otsu thresholding → morphological denoising
   - `detectCharacters()`: Contour detection → bounding box extraction → equals sign merging
   - `recognizeCharacter()`: **Morphological feature-based classification**
   - `evaluateExpression()`: Simple stack-based calculator

### Recognition Pipeline

```
Input Image
    ↓
[Preprocessing] → Binary image (Otsu + morphology)
    ↓
[Character Detection] → Contour extraction → Bounding boxes → Merge "=" lines
    ↓
[Character Recognition] → Feature extraction → Rule-based classification
    ↓
[Expression Evaluation] → Parse operators → Calculate result
    ↓
Output: (formula_string, numeric_result)
```

### Feature-Based Recognition System

**Core Technique**: The system uses **hand-crafted morphological features** instead of ML/OCR:

#### Feature Set (formula_recognizer.cpp:55-147)
- **Density**: Pixel ratio in 28×40 normalized ROI
- **Aspect Ratio**: Width/height ratio
- **Hole Count**: Number of enclosed regions (contours - 1)
- **Vertical Distribution**: Pixel ratios in top/middle/bottom thirds (TMB ratios)

#### Recognition Rules
- **Operators**: Shape-based (e.g., "=" has h<20, AR>1.8, density>0.4)
- **Digits**: Combination of holes + TMB distribution:
  - "8": 2+ holes, high density
  - "0": 1 hole, balanced TMB
  - "1": Low AR, low density, no holes
  - "6": 1 hole, bottom-heavy
  - "9": 1 hole, top-heavy

#### Special Processing
- **Equals sign merging** (lines 178-206): Detects two horizontal lines (h<5, AR>3) with small x-diff and y-gap (3-15px), merges into single "=" character

### CMake Configuration

**Key Settings**:
- C++11 standard required
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` (user requirement for IDE support)
- Single executable target: `formula_recognition_cli`
- Links against OpenCV only (no Tesseract/other OCR libs)

### Chinese Context
- Console output in Chinese (e.g., "开始图像预处理...")
- Comments use Chinese for documentation
- Code identifiers in English
- User-facing messages in Chinese

## Development Notes

### Current Limitations
1. **No ML/Deep Learning**: Pure morphological feature matching
2. **Limited Character Set**: Only digits 0-9 and operators +, -, ×(x), ÷(/), =
3. **Simple Expression Evaluator**: Left-to-right parsing, no operator precedence
4. **Single Formula**: One expression per image (no multi-formula support)
5. **No Rotation Handling**: Assumes upright text

### Extending the System

**To add new characters**:
1. Collect morphological features from sample images using `--debug`
2. Add classification rules in `recognizeCharacter()` (line 55)
3. Test with `--debug` to tune thresholds

**To improve accuracy**:
- Tune feature thresholds based on debug output
- Add more discriminative features
- Consider template matching for difficult characters

**To support rotation**:
- Detect text orientation before preprocessing
- Add `minAreaRect` analysis for skew correction

### User Requirements Compliance
- Uses CMake with `CMAKE_EXPORT_COMPILE_COMMANDS ON` ✓
- No creation of extra files (single binary output) ✓
- Chinese documentation and console output ✓
- Built on existing codebase (no new scripts/folders) ✓
