## Unsorted Notes
- set bounding boxes for each line, which will be an equation
- within the line, bound each character. map it to a ASCII character
- need to note position and size of each character, especially in relation to other characters
	- think exponents which are raised above the x-height (in an IDEAL world)
	- think subscript which are below the baseline
	- things like fractions have a horizontal line that separates the numerator and denominator
	- square roots are some characters with a character bounding them (radical sign)

- start with a simple equation, like 2+2=4

## characters:
- numbers (0-9)
- variables (a-z, A-Z)
- decimal point (.)
- sign (+, -)
- equal sign (=)
- greek letters/alphabet (α, β, γ, etc.)

## operators:
- addition (+)
- subtraction (-)
- multiplication (*, x, •) (two variables next to each other; e.g., AB or 2x is really A times B or 2 times x)
- division (/, ÷) (two variables separated by a horizontal line; e.g., A/B or 2/x is really A divided by B or 2 divided by x) (fractions)

- parentheses ((), [], {}) (a variable or expression inside; e.g., (x+2) is really x plus 2)

- exponents (^) (a variable raised above the x-height; e.g., x^2 is really x squared)
- square roots (√) (a radical sign with a variable inside; e.g., √x is really the square root of x)

- trigonometric functions (sin, cos, tan, etc.) (a function with a variable inside; e.g., sin(x) is really the sine of x)
- logarithms (log) (a function with a variable inside; e.g., log(x) is really the logarithm of x)

- vector notation (→) (a vector with a variable inside; e.g., →x is really the vector x)
- matrix notation ([, ]) (a matrix with variables inside; e.g., [x, y] is really the matrix x, y)


-- At this point only characters (numbers, math symbols) are present in the image we are working with
-- Segment by line, each object is assigned a line
-- Use position and character value to recognize equals, multi-digit numbers (i.e., if 1 and 2 are together with no object in between and their distance is reasonably small they can be assumed to form ‘12’)
-- Form math equation using LaTeX
-- Generate an output image! (https://github.com/goldsborough/latexpp)

Get contours of each numeric character
# of contours is a very rough tell:
1 contour: 1, 2, 3, 5, 7
2 contours: 0, 4, 6, 9
3 contours: 8
Roundness
Angle for adjustments
# of edges
# of corners

other resources
