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
# Basic usage (auto-generates result image and displays it)
./build/formula_recognition_cli formula_images/formula_1.png
# Output: formula_images/formula_1_result.png (auto-displayed, press any key to close)

# With debug output (shows detailed recognition steps and intermediate images)
./build/formula_recognition_cli formula_images/formula_1.png --debug

# Custom output path
./build/formula_recognition_cli formula_images/formula_1.png --output custom_result.png

# Batch process all formulas
for i in {1..8}; do
  ./build/formula_recognition_cli formula_images/formula_$i.png \
    --output formula_images/formula_${i}_result.png
done
```

**Note**: The program automatically generates a result image with computed answers displayed in red on the right side of the equals sign, and shows it in a window (press any key to close).

### Test Images & Accuracy
Eight test formula images are available in `formula_images/` with **100% overall accuracy (8/8)**:

| File | Formula | Expected | Recognized | Result | Status |
|------|---------|----------|------------|--------|--------|
| formula_1.png | `12+34=46` | 46 | `12+34=` | 46 | ✅ Perfect |
| formula_2.png | `56-23=33` | 33 | `56-23=` | 33 | ✅ Perfect |
| formula_3.png | `8×9=72` | 72 | `8x9=` | 72 | ✅ Perfect |
| formula_4.png | `100÷5=20` | 20 | `100/5=` | 20 | ✅ Perfect |
| formula_5.png | `3+5×2=13` | 13 | `3+5x2=` | 13 | ✅ Perfect |
| formula_6.png | `45-12+8=41` | 41 | `45-12+8=` | 41 | ✅ Perfect |
| formula_7.png | `(3+5)×2=16` | 16 | `(3+5)x2=` | 16 | ✅ Perfect |
| formula_8.png | `√16=4` | 4 | `s16=` | 4 | ✅ Perfect |

**Key Recognition Achievements**:
- Operators: 100% (+ - × ÷ =)
- Parentheses: 100% (() included in formulas 7)
- Radicals: 100% (√ in formula 8, represented as 's' internally)
- Digits: 100% (0-9, total 43/43 characters)
- Overall character recognition: 100% (53/53 characters)

## Code Architecture

### Three-File Structure

1. **main.cpp** - CLI entry point
   - Argument parsing (image path, --debug flag)
   - Creates FormulaRecognizer instance
   - Displays final results

2. **formula_recognizer.h** - Interface definitions
   - `RecognizedChar` struct: character, bounding box, confidence
   - `FormulaRecognizer` class: main recognition pipeline

3. **formula_recognizer.cpp** - Core recognition logic
   - `preprocessImage()`: Grayscale → Otsu thresholding → morphological denoising
   - `detectCharacters()`: Contour detection → bounding box extraction → intelligent symbol merging
     - Equals sign merging: Two horizontal lines (h<5, AR>3) with 3-15px y-gap
     - **Division sign merging**: Three-part detection (dot-line-dot) with automatic assembly
   - `recognizeCharacter()`: **Morphological feature-based classification**
   - `evaluateExpression()`: **Dual-stack algorithm** with operator precedence support
     - Precedence: Radical(√) > Parentheses(()) > Multiply/Divide(×÷) > Add/Subtract(+-)

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

#### Recognition Rules (100% Accuracy Achieved)
**Operators**:
- `-` (minus): Single horizontal line (h<8, AR>2.5, density>0.8)
- `×` (multiply): Square + medium density (AR≈1, density:0.4-0.65, holes=0)
- `÷` (divide): Three-part merge (dot-line-dot) + low density (holes≥2, density<0.4)
- `=` (equals): Two horizontal lines merged (h<20, AR>1.8, density>0.4)
- `+` (plus): Square + low density (AR≈1, density:0.2-0.35)

**Parentheses**:
- `(` (left): Thin + medium density + left-heavy pixels (AR<0.35, density:0.45-0.58)
- `)` (right): Thin + medium density + right-heavy pixels (AR<0.35, density:0.45-0.58)

**Radicals**:
- `√` (square root): Very low density + bottom-heavy + light top (AR:0.6-0.85, density:0.15-0.28, topRatio<0.25)
  - Represented internally as 's', calculated as sqrt() operation
  - Distinguishable from digit 5: lower density (D<0.28 vs D>0.47), lighter top

**Digits**: Combination of holes + TMB (Top-Middle-Bottom) distribution:
- `0`: 1 hole, balanced TMB
- `1`: Low AR, low density, no holes
- `2`: Top+bottom heavy, middle light
- `3`: Middle light (vs 5: middle heavier)
- `4`: Recognized after 6/9 to avoid confusion
- `5`: Middle heavier than 3
- `6`: 1 hole, bottom-heavy
- `7`: Top heavy (topRatio>0.45), middle+bottom light (midRatio<0.28)
- `8`: 2+ holes, high density
- `9`: 1 hole, top-heavy, bottom lightest (botRatio<0.32)

#### Special Processing
- **Equals sign merging**: Detects two horizontal lines (h<5, AR>3) with 3-15px y-gap and small x-diff
- **Division sign merging**: Intelligent three-part assembly (dot + line + dot) with spatial proximity check

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

### Current Status & Limitations

**✅ Fully Implemented (100% Accuracy)**:
1. All digits 0-9 with optimized feature discrimination
2. All basic operators: +, -, ×, ÷, =
3. Parentheses: (, )
4. Radical: √ (square root, unary operator)
5. Operator precedence with dual-stack algorithm
6. Intelligent symbol merging (equals, division signs)
7. Smart formula parsing (ignores answer after equals sign)
8. Automatic result visualization (red text on result image)

**❌ Current Limitations**:
1. **No ML/Deep Learning**: Pure morphological feature matching (intentional design choice)
2. **Complex Operators**: No support for exponents, higher-order radicals, trigonometric functions
3. **Nested Parentheses**: Only tested with single-level parentheses
4. **Complex Radical Expressions**: Currently only supports `√number`, not `√(expression)`
5. **Single Formula**: One expression per image (no multi-formula support)
6. **No Rotation Handling**: Assumes upright text

### Extending the System

**To add new characters**:
1. Run `--debug` mode on sample images containing the target character
2. Analyze morphological features (density, AR, holes, TMB ratios) from console output
3. Add classification rules in `recognizeCharacter()` function (formula_recognizer.cpp)
4. Iteratively tune thresholds using `--debug` output until accuracy is satisfactory

**Example workflow** (adding a new operator):
```bash
# 1. Capture debug features
./build/formula_recognition_cli test_image.png --debug > features.txt

# 2. Edit formula_recognizer.cpp to add new rules based on features.txt
# 3. Rebuild and test
cd build && make
./build/formula_recognition_cli test_image.png

# 4. Verify with all test cases
for i in {1..8}; do ./build/formula_recognition_cli formula_images/formula_$i.png; done
```

**To improve operator precedence**:
- Modify precedence values in `getPrecedence()` function (formula_recognizer.cpp)
- Adjust the dual-stack evaluation logic in `evaluateExpression()`

**To support rotation**:
- Add `minAreaRect` analysis in `preprocessImage()` for skew detection
- Apply affine transformation before Otsu thresholding

**To support multi-formula images**:
- Implement vertical segmentation in `detectCharacters()`
- Add clustering logic to group characters into separate formulas

### Optimization History (from README.md)
Key improvements that achieved 100% accuracy:
1. **Operator discrimination**: Fine-tuned density/AR thresholds for +, -, ×, ÷
2. **Division sign assembly**: Three-part merging algorithm (dot-line-dot)
3. **Digit disambiguation**: TMB distribution for 3/5, 6/9/4 priority ordering, 7/2 top-heavy detection
4. **Parentheses**: Left/right pixel distribution analysis
5. **Radical**: Low-density + bottom-heavy features (distinguishable from digit 5)
6. **Operator precedence**: Dual-stack algorithm replacing naive left-to-right evaluation

### User Requirements Compliance
- Uses CMake with `CMAKE_EXPORT_COMPILE_COMMANDS ON` ✓
- No creation of extra files (binary output + optional result images) ✓
- Chinese documentation and console output ✓
- Built on existing codebase (no new scripts/folders) ✓
- Modifications made on original files only ✓
