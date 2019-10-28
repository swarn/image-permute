#ifndef TRANSFORMATIONS_HPP

#include "array2d.hpp"
#include "pixels.hpp"

// Return an image such that the darkest pixel in the palette is in the same
// position as the darkest pixel in the input picture, and so on for all
// pixels. Ignores color, only uses luminance/brightness.
array2d<pixel> match_ascending(
    array2d<pixel> const & picture, array2d<pixel> const & palette);


// Pick two random pixels in the output image. Measure the perceived color
// distance between the output pixels and their corresponding pixels in the
// input. If the the sum of the squared differences would be smaller if you
// swap the pixels, then swap the pixels.
//
// A single pass is a number of random compare-and-swaps equal to the number
// of pixels. Each pixel will particpate in two possible swaps.
void compare_and_swap(
    array2d<pixel> const & input, array2d<pixel> & output, int passes);


// As compare_and_swap, but instead of comparing output pixels directly to
// their corresponding input pixels, compare them to the average color of a
// small region around the input pixel.
void compare_and_swap_dithered(
    array2d<pixel> const & input, array2d<pixel> & output, int passes);

#define TRANSFORMATIONS_HPP
#endif
