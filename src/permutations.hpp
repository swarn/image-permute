#ifndef PERMUTATIONS_HPP
#define PERMUTATIONS_HPP

#include "XoshiroCpp.hpp"

#include "array2d.hpp"
#include "colors.hpp"


using permute_rng_type = XoshiroCpp::Xoshiro256StarStar;

// Permute the pixels in the `output` image to more closely resemble the
// `reference` image, based on brightness.
//
// After exeuction, the n-th brightest pixel in `output` is in the same
// position as the n-th brightest pixel in the `input` image. This ignores
// all aspects of color other than brightness.
void match_ascending(array2d<rgb> const & input, array2d<rgb> & output);


// Permute the pixels in the `output` image to more closely resemble the
// `reference` image, based on perceived color difference.
//
// Pick two random pixels in the output image. Measure the perceived color
// distance between the output pixels and their corresponding pixels in the
// input. If the the sum of the squared differences would be smaller if the
// pixels are swapped, then do it.
//
// A single pass is a number of random compare-and-swaps equal to the number
// of pixels. Each pixel will particpate in two compare-and-swaps.
void compare_and_swap(
    array2d<rgb> const & input,
    array2d<rgb> & output,
    int passes,
    permute_rng_type & rng);


// Permute the pixels in the `output` image to more closely resemble the
// `reference` image, based on perceived color difference.
//
// As `compare_and_swap`, but instead of comparing output pixels directly to
// their corresponding input pixels, compare them to the average color of a
// small region around the input pixel.
void compare_and_swap_dithered(
    array2d<rgb> const & input,
    array2d<rgb> & output,
    int passes,
    permute_rng_type & rng);

#endif
