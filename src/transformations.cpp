#include "transformations.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>


array2d<pixel>
match_ascending(array2d<pixel> const & picture, array2d<pixel> const & palette)
{
    std::vector<int> picture_indices(picture.size);
    std::iota(picture_indices.begin(), picture_indices.end(), 0);
    std::vector<int> palette_indices(picture.size);
    std::iota(palette_indices.begin(), palette_indices.end(), 0);

    std::sort(picture_indices.begin(), picture_indices.end(),
        [&](int lhs, int rhs){ return picture[lhs].lab.L < picture[rhs].lab.L; });
    std::sort(palette_indices.begin(), palette_indices.end(),
        [&](int lhs, int rhs) { return palette[lhs].lab.L < palette[rhs].lab.L; });

    array2d<pixel> result(picture.rows, picture.cols);
    for (int i = 0; i < result.size; i++)
        result[picture_indices[i]] = palette[palette_indices[i]];

    return result;
};

void compare_and_swap(array2d<pixel> const & input, array2d<pixel> & output, int passes)
{
    std::vector<int> here_idxs(input.size);
    std::iota(here_idxs.begin(), here_idxs.end(), 0);
    std::vector<int> there_idxs(input.size);
    std::iota(there_idxs.begin(), there_idxs.end(), 0);
    std::random_device r;
    std::mt19937_64 gen{r()};

    for (int pass = 0; pass < passes; pass++)
    {
        std::shuffle(here_idxs.begin(), here_idxs.end(), gen);
        std::shuffle(there_idxs.begin(), there_idxs.end(), gen);

        for (int i = 0; i < input.size; i++)
        {
            auto here = here_idxs[i];
            auto there = there_idxs[i];
            auto current =
                diff2(output[here], input[here]) + diff2(output[there], input[there]);
            auto swapped =
                diff2(output[here], input[there]) + diff2(output[there], input[here]);
            if (swapped < current)
                std::swap(output[here], output[there]);
        }
    }
}


// There are endless ways to optimize convolutions. This is defintely not the
// best implementation; it's probably not the worst.
//
// For handling the edges, neither reflecting or extending seem right to me, so
// instead I truncate the kernel and modify the weights at the edges.
struct neighbor_info
{
    rgb_float rgb;
    float weight;
};

neighbor_info blur(array2d<pixel> const & array, int row, int col)
{
    constexpr int TOP = 1;
    constexpr int BOTTOM = 2;
    constexpr int LEFT = 4;
    constexpr int RIGHT = 8;

    int const region =
        (row == 0              ? TOP    : 0) |
        (row == array.rows - 1 ? BOTTOM : 0) |
        (col == 0              ? LEFT   : 0) |
        (col == array.cols - 1 ? RIGHT  : 0);

    switch (region)
    {
    case TOP:
        return {array(row    , col - 1).rgb * 2.0f +
                array(row    , col + 1).rgb * 2.0f +
                array(row + 1, col - 1).rgb * 1.0f +
                array(row + 1, col    ).rgb * 2.0f +
                array(row + 1, col + 1).rgb * 1.0f,
                8.0f};
    case BOTTOM:
        return {array(row - 1, col - 1).rgb * 1.0f +
                array(row - 1, col    ).rgb * 2.0f +
                array(row - 1, col + 1).rgb * 1.0f +
                array(row    , col - 1).rgb * 2.0f +
                array(row    , col + 1).rgb * 2.0f,
                8.0f};
    case LEFT:
        return {array(row - 1, col    ).rgb * 2.0f +
                array(row - 1, col + 1).rgb * 1.0f +
                array(row    , col + 1).rgb * 2.0f +
                array(row + 1, col    ).rgb * 2.0f +
                array(row + 1, col + 1).rgb * 1.0f,
                8.0f};
    case RIGHT:
        return {array(row - 1, col - 1).rgb * 1.0f +
                array(row - 1, col    ).rgb * 2.0f +
                array(row    , col - 1).rgb * 2.0f +
                array(row + 1, col - 1).rgb * 1.0f +
                array(row + 1, col    ).rgb * 2.0f,
                8.0f};
    case TOP | LEFT:
        return {array(row    , col + 1).rgb * 2.0f +
                array(row + 1, col    ).rgb * 2.0f +
                array(row + 1, col + 1).rgb * 1.0f,
                5.0f};
    case TOP | RIGHT:
        return {array(row    , col - 1).rgb * 2.0f +
                array(row + 1, col - 1).rgb * 1.0f +
                array(row + 1, col    ).rgb * 2.0f,
                5.0f};
    case BOTTOM | LEFT:
        return {array(row - 1, col    ).rgb * 2.0f +
                array(row - 1, col + 1).rgb * 1.0f +
                array(row    , col + 1).rgb * 2.0f,
                5.0f};
    case BOTTOM | RIGHT:
        return {array(row - 1, col - 1).rgb * 1.0f +
                array(row - 1, col    ).rgb * 2.0f +
                array(row    , col - 1).rgb * 2.0f,
                5.0f};
    default:
        return {array(row - 1, col - 1).rgb * 1.0f +
                array(row - 1, col    ).rgb * 2.0f +
                array(row - 1, col + 1).rgb * 1.0f +
                array(row    , col - 1).rgb * 2.0f +
                array(row    , col + 1).rgb * 2.0f +
                array(row + 1, col - 1).rgb * 1.0f +
                array(row + 1, col    ).rgb * 2.0f +
                array(row + 1, col + 1).rgb * 1.0f,
                12.0f};
    }
}

void compare_and_swap_dithered(
    array2d<pixel> const & input, array2d<pixel> & output, int passes)
{
    std::vector<int> here_idxs(input.size);
    std::iota(here_idxs.begin(), here_idxs.end(), 0);
    std::vector<int> there_idxs(input.size);
    std::iota(there_idxs.begin(), there_idxs.end(), 0);
    std::random_device r;
    std::mt19937_64 gen{r()};

    for (int pass = 0; pass < passes; pass++)
    {
        std::shuffle(here_idxs.begin(), here_idxs.end(), gen);
        std::shuffle(there_idxs.begin(), there_idxs.end(), gen);

        int num_swaps = 0;

        for (size_t j = 0; j < here_idxs.size(); j++)
        {
            auto here = here_idxs[j];
            auto there = there_idxs[j];
            auto here_col = here % input.cols;
            auto here_row = here / input.cols;
            auto there_col = there % input.cols;
            auto there_row = there / input.cols;

            auto [here_neighbors, here_weight] = blur(output, here_row, here_col);
            auto [there_neighbors, there_weight] = blur(output, there_row, there_col);

            auto here_now = (here_neighbors + output[here].rgb * 4.0)
                          * (1.0 / (4.0 + here_weight));
            auto here_swapped = (here_neighbors + output[there].rgb * 4.0)
                              * (1.0 / (4.0 + here_weight));
            auto there_now = (there_neighbors + output[there].rgb * 4.0)
                           * (1.0 / (4.0 + there_weight));
            auto there_swapped = (there_neighbors + output[here].rgb * 4.0)
                               * (1.0 / (4.0 + there_weight));
            auto current = diff2(lab(here_now), input[here].lab) +
                           diff2(lab(there_now), input[there].lab);
            auto swapped = diff2(lab(here_swapped), input[here].lab) +
                           diff2(lab(there_swapped), input[there].lab);
            if (swapped < current)
            {
                std::swap(output(here_row, here_col), output(there_row, there_col));
                num_swaps++;
            }
        }

        float swap_freq = num_swaps * 1.0 / output.size;
        std::cout << "pass " << pass << ": "
                  << num_swaps << '/' << output.size
                  << " " << swap_freq;

        if (pass % 10 == 0)
        {
            double accum = 0;
            for (int i = 0; i < output.size; i++)
                accum += diff2(output[i].lab, input[i].lab);

            double rms = std::sqrt(accum / output.size);
            std::cout << " rms: " << rms;
        }

        std::cout << '\n';
        num_swaps = 0;
    }
}
