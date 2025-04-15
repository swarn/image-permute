#include "permutations.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

#include "array2d.hpp"
#include "colors.hpp"


void match_ascending(array2d<rgb> const & input, array2d<rgb> & output)
{
    // Convert to CIELAB for luminance.
    auto const input_lab = array2d<lab> {input};
    auto const output_lab = array2d<lab> {output};

    // Sort the indices of input picture based on the luminance of the pixel
    // to which they refer.
    std::vector<size_t> input_indices(input.size());
    std::iota(input_indices.begin(), input_indices.end(), 0);
    std::sort(input_indices.begin(), input_indices.end(), [&](auto lhs, auto rhs) {
        return input_lab[lhs].L < input_lab[rhs].L;
    });

    // Sort the indices of the output based on the luminance of the pixel
    // to which they refer.
    std::vector<size_t> output_indices(output.size());
    std::iota(output_indices.begin(), output_indices.end(), 0);
    std::sort(output_indices.begin(), output_indices.end(), [&](auto lhs, auto rhs) {
        return output_lab[lhs].L < output_lab[rhs].L;
    });

    // The darkest pixel in the original `output` is copied to the position of
    // the darkest pixel in the `input` image, and so on.
    auto const original_output = output;
    for (size_t i = 0; i < output.size(); i++)
        output[input_indices[i]] = original_output[output_indices[i]];
};


void compare_and_swap(
    array2d<rgb> const & input,
    array2d<rgb> & output,
    int passes,
    permute_rng_type & rng)
{
    // Convert the images into CIELAB color space to measure perceived
    // differences.
    auto const input_lab = array2d<lab> {input};
    auto output_lab = array2d<lab> {output};

    // Each pixel will be `here` once per pass, and `there` once per pass.
    std::vector<size_t> here_idxs(input.size());
    std::iota(here_idxs.begin(), here_idxs.end(), 0);
    std::vector<size_t> there_idxs(input.size());
    std::iota(there_idxs.begin(), there_idxs.end(), 0);

    for (int pass = 0; pass < passes; pass++)
    {
        std::shuffle(here_idxs.begin(), here_idxs.end(), rng);
        std::shuffle(there_idxs.begin(), there_idxs.end(), rng);

        for (size_t i = 0; i < input.size(); i++)
        {
            auto here = here_idxs[i];
            auto there = there_idxs[i];

            // What is the sum of the squared differences between these output
            // pixels and the corresponding input pixels?
            auto current = diff2(output_lab[here], input_lab[here]) +
                diff2(output_lab[there], input_lab[there]);

            // If the pixels were swapped, what's the sum of differences?
            auto swapped = diff2(output_lab[here], input_lab[there]) +
                diff2(output_lab[there], input_lab[here]);

            if (swapped < current)
            {
                std::swap(output[here], output[there]);
                std::swap(output_lab[here], output_lab[there]);
            }
        }
    }
}


namespace {

//////////////////////////////////////////////////////////////////////////////
// Dithering
//
// Like `compare_and_swap` above, the strategy is to pick random pairs of
// pixels and swap them if that reduces the total perceived difference
// between those pixels and the input image. The difference is that, in
// `compare_and_swap_dithered`, the output pixels are blurred with the 3x3
// Guassian. This is slightly strange: the blurred neighborhood is compared
// with a single input pixel. But it works! Not only does it dither, but it
// still manages to capture pixel-level detail, e.g. fine lines.
//
// Because the algorithm checks randomly-selected pairs of pixels, pre-blurring
// the whole image isn't useful. But, computing the weighted sum of the
// neighbors around each pixel is useful: this sum can be used to compute both
// the blurred swapped and non-swapped pixels. If the the pixels are swapped,
// then all eight neighbors of that pixel must be updated. This reduces the FP
// operations used by ~90%. However, that doesn't translate directly to a
// performance increase: the random memory access pattern means that FP
// computation is not the execution bottleneck.
//
// For handling the edges, neither reflecting or extending seem right to
// me, so instead, truncate the kernel at the edges. This means there are
// nine kernels to select from based on the location of the pixel.

constexpr uint8_t TOP = 1;
constexpr uint8_t BOTTOM = 2;
constexpr uint8_t LEFT = 4;
constexpr uint8_t RIGHT = 8;
constexpr uint8_t edges(size_t row, size_t col, size_t rows, size_t cols)
{
    // clang-format off
    return (row == 0        ? TOP    : 0U) |
           (row == rows - 1 ? BOTTOM : 0U) |
           (col == 0        ? LEFT   : 0U) |
           (col == cols - 1 ? RIGHT  : 0U);
    // clang-format on
}

// Convolve the given pixel with a Gaussian with a hole in the middle, i.e.:
//
//     1  2  1
//     2  0  2
//     1  2  1
//
// This result can be added to a central pixel to get the complete convolution.
rgb_float blur_around(array2d<rgb> const & array, size_t neighborhood)
{
    size_t const col = neighborhood % array.cols;
    size_t const row = neighborhood / array.cols;

    // Determine which of nine kernels to use, based on edge adjacency.
    uint8_t const region = edges(row, col, array.rows, array.cols);

    // Ignore the degenerate cases where a pixel is adjacent to more than two
    // edges, which only occur in images with a single row or column.
    switch (region)
    {
    case TOP: // clang-format off
        return rgb_float{array(row    , col - 1)} * 2.0 +
               rgb_float{array(row    , col + 1)} * 2.0 +
               rgb_float{array(row + 1, col - 1)} * 1.0 +
               rgb_float{array(row + 1, col    )} * 2.0 +
               rgb_float{array(row + 1, col + 1)} * 1.0;
    case BOTTOM:
        return rgb_float{array(row - 1, col - 1)} * 1.0 +
               rgb_float{array(row - 1, col    )} * 2.0 +
               rgb_float{array(row - 1, col + 1)} * 1.0 +
               rgb_float{array(row    , col - 1)} * 2.0 +
               rgb_float{array(row    , col + 1)} * 2.0;
    case LEFT:
        return rgb_float{array(row - 1, col    )} * 2.0 +
               rgb_float{array(row - 1, col + 1)} * 1.0 +
               rgb_float{array(row    , col + 1)} * 2.0 +
               rgb_float{array(row + 1, col    )} * 2.0 +
               rgb_float{array(row + 1, col + 1)} * 1.0;
    case RIGHT:
        return rgb_float{array(row - 1, col - 1)} * 1.0 +
               rgb_float{array(row - 1, col    )} * 2.0 +
               rgb_float{array(row    , col - 1)} * 2.0 +
               rgb_float{array(row + 1, col - 1)} * 1.0 +
               rgb_float{array(row + 1, col    )} * 2.0;
    case TOP | LEFT:
        return rgb_float{array(row    , col + 1)} * 2.0 +
               rgb_float{array(row + 1, col    )} * 2.0 +
               rgb_float{array(row + 1, col + 1)} * 1.0;
    case TOP | RIGHT:
        return rgb_float{array(row    , col - 1)} * 2.0 +
               rgb_float{array(row + 1, col - 1)} * 1.0 +
               rgb_float{array(row + 1, col    )} * 2.0;
    case BOTTOM | LEFT:
        return rgb_float{array(row - 1, col    )} * 2.0 +
               rgb_float{array(row - 1, col + 1)} * 1.0 +
               rgb_float{array(row    , col + 1)} * 2.0;
    case BOTTOM | RIGHT:
        return rgb_float{array(row - 1, col - 1)} * 1.0 +
               rgb_float{array(row - 1, col    )} * 2.0 +
               rgb_float{array(row    , col - 1)} * 2.0;
    default:
        return rgb_float{array(row - 1, col - 1)} * 1.0 +
               rgb_float{array(row - 1, col    )} * 2.0 +
               rgb_float{array(row - 1, col + 1)} * 1.0 +
               rgb_float{array(row    , col - 1)} * 2.0 +
               rgb_float{array(row    , col + 1)} * 2.0 +
               rgb_float{array(row + 1, col - 1)} * 1.0 +
               rgb_float{array(row + 1, col    )} * 2.0 +
               rgb_float{array(row + 1, col + 1)} * 1.0;
    } // clang-format on
}


// Using the sum of neighbors provided by `blur_around`, fill in a center value
// and get the final blurred pixel.
rgb_float
blur_for(array2d<rgb_float> const & neighborhood, size_t pos, rgb const & center)
{
    size_t const col = pos % neighborhood.cols;
    size_t const row = pos / neighborhood.cols;

    // Sum this pixel with the pos to get the blurred version.
    auto blurred = neighborhood[pos] + rgb_float {center} * 4.0;

    // Determine the correct kernel normalization, based on edge adjacency.
    // Ignore the degenerate cases where a pixel is adjacent to more than two
    // edges, which only occur in images with a single row or column.
    switch (edges(row, col, neighborhood.rows, neighborhood.cols))
    {
    case TOP:
    case BOTTOM:
    case LEFT:
    case RIGHT:
        return blurred * (1.0F / 12.0F);
    case TOP | LEFT:
    case TOP | RIGHT:
    case BOTTOM | LEFT:
    case BOTTOM | RIGHT:
        return blurred * (1.0F / 9.0F);
    default:
        return blurred * (1.0F / 16.0F);
    }
}


// When a pixel has changed by the given `delta`, update the eight neighbors
// values accordingly.
void update_blur(
    array2d<rgb_float> & array, size_t neighborhood, rgb_float const & delta)
{
    size_t const col = neighborhood % array.cols;
    size_t const row = neighborhood / array.cols;

    // Determine which neighbors to update, based on edge adjacency.  Ignore
    // the degenerate cases where a pixel is adjacent to more than two edges,
    // which only occur in images with a single row or column.
    switch (edges(row, col, array.rows, array.cols))
    {
    case TOP: // clang-format off
        array(row    , col - 1) += delta * 2.0;
        array(row    , col + 1) += delta * 2.0;
        array(row + 1, col - 1) += delta * 1.0;
        array(row + 1, col    ) += delta * 2.0;
        array(row + 1, col + 1) += delta * 1.0;
        return;
    case BOTTOM:
        array(row - 1, col - 1) += delta * 1.0;
        array(row - 1, col    ) += delta * 2.0;
        array(row - 1, col + 1) += delta * 1.0;
        array(row    , col - 1) += delta * 2.0;
        array(row    , col + 1) += delta * 2.0;
        return;
    case LEFT:
        array(row - 1, col    ) += delta * 2.0;
        array(row - 1, col + 1) += delta * 1.0;
        array(row    , col + 1) += delta * 2.0;
        array(row + 1, col    ) += delta * 2.0;
        array(row + 1, col + 1) += delta * 1.0;
        return;
    case RIGHT:
        array(row - 1, col - 1) += delta * 1.0;
        array(row - 1, col    ) += delta * 2.0;
        array(row    , col - 1) += delta * 2.0;
        array(row + 1, col - 1) += delta * 1.0;
        array(row + 1, col    ) += delta * 2.0;
        return;
    case TOP | LEFT:
        array(row    , col + 1) += delta * 2.0;
        array(row + 1, col    ) += delta * 2.0;
        array(row + 1, col + 1) += delta * 1.0;
        return;
    case TOP | RIGHT:
        array(row    , col - 1) += delta * 2.0;
        array(row + 1, col - 1) += delta * 1.0;
        array(row + 1, col    ) += delta * 2.0;
        return;
    case BOTTOM | LEFT:
        array(row - 1, col    ) += delta * 2.0;
        array(row - 1, col + 1) += delta * 1.0;
        array(row    , col + 1) += delta * 2.0;
        return;
    case BOTTOM | RIGHT:
        array(row - 1, col - 1) += delta * 1.0;
        array(row - 1, col    ) += delta * 2.0;
        array(row    , col - 1) += delta * 2.0;
        return;
    default:
        array(row - 1, col - 1) += delta * 1.0;
        array(row - 1, col    ) += delta * 2.0;
        array(row - 1, col + 1) += delta * 1.0;
        array(row    , col - 1) += delta * 2.0;
        array(row    , col + 1) += delta * 2.0;
        array(row + 1, col - 1) += delta * 1.0;
        array(row + 1, col    ) += delta * 2.0;
        array(row + 1, col + 1) += delta * 1.0;
        return;
    } // clang-format on
}

} // end anonymous namespace


void compare_and_swap_dithered(
    array2d<rgb> const & input,
    array2d<rgb> & output,
    int passes,
    permute_rng_type & rng)
{
    std::vector<size_t> here_idxs(input.size());
    std::iota(here_idxs.begin(), here_idxs.end(), 0);
    std::vector<size_t> there_idxs(input.size());
    std::iota(there_idxs.begin(), there_idxs.end(), 0);

    auto const input_lab = array2d<lab> {input};

    array2d<rgb_float> blurred_neighbors(output.rows, output.cols);
    for (size_t i = 0; i < output.size(); i++)
        blurred_neighbors[i] = blur_around(output, i);

    for (int pass = 0; pass < passes; pass++)
    {
        std::shuffle(here_idxs.begin(), here_idxs.end(), rng);
        std::shuffle(there_idxs.begin(), there_idxs.end(), rng);

        int num_swaps = 0;

        for (size_t i = 0; i < here_idxs.size(); i++)
        {
            auto here = here_idxs[i];
            auto there = there_idxs[i];

            // The access pattern is random, but known in advance. Prefetching
            // reduces execution time by ~25%.
            constexpr int ahead = 4;
            auto future_here = i < here_idxs.size() ? here_idxs[i + ahead] : 0;
            auto future_there = i < there_idxs.size() ? there_idxs[i + ahead] : 0;
            __builtin_prefetch(&output[future_here]);
            __builtin_prefetch(&output[future_there]);
            __builtin_prefetch(&blurred_neighbors[future_here]);
            __builtin_prefetch(&blurred_neighbors[future_there]);
            __builtin_prefetch(&input_lab[future_here]);
            __builtin_prefetch(&input_lab[future_there]);

            // What does this output pixel look like with a little blur?  What
            // would it look like if swapped with the other pixel?
            auto here_now = blur_for(blurred_neighbors, here, output[here]);
            auto here_swapped = blur_for(blurred_neighbors, here, output[there]);

            // Ask the same two questions for the other pixel.
            auto there_now = blur_for(blurred_neighbors, there, output[there]);
            auto there_swapped = blur_for(blurred_neighbors, there, output[here]);

            // Now ask, what is the sum of the squared differences between
            // the input and output images at these two pixels, both without
            // and with swapping?
            auto current = diff2(lab {here_now}, input_lab[here]) +
                diff2(lab {there_now}, input_lab[there]);
            auto swapped = diff2(lab {here_swapped}, input_lab[here]) +
                diff2(lab {there_swapped}, input_lab[there]);

            if (swapped < current)
            {
                auto delta = rgb_float {output[here]} - rgb_float {output[there]};
                auto nabla = rgb_float {output[there]} - rgb_float {output[here]};
                update_blur(blurred_neighbors, there, delta);
                update_blur(blurred_neighbors, here, nabla);

                std::swap(output[here], output[there]);
                num_swaps++;
            }
        }

        double const swap_freq = num_swaps / static_cast<double>(output.size());
        std::cout << "pass " << pass << ": ";
        std::cout << num_swaps << '/' << output.size() << " " << swap_freq;

        if (pass % 10 == 0)
        {
            float accum = 0;
            for (size_t i = 0; i < output.size(); i++)
                accum += diff2(lab {output[i]}, input_lab[i]);

            float const rms = std::sqrt(accum / static_cast<float>(output.size()));
            std::cout << " rms: " << rms;
        }

        std::cout << '\n';
    }
}
