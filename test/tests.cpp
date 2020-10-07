#include <catch2/catch.hpp>

#include <algorithm>
#include <bitset>
#include <cmath>
#include <vector>

#include "colors.hpp"
#include "grid.hpp"
#include "hilbert.hpp"


TEST_CASE("make_palette makes all colors")
{
    auto palette = make_palette(rgb::num_colors);
    REQUIRE(palette.size() == rgb::num_colors);

    std::vector<unsigned> as_ints(palette.begin(), palette.end());
    std::sort(as_ints.begin(), as_ints.end());

    bool has_all = true;
    for (size_t i = 0; i < as_ints.size(); i++)
        has_all = has_all and (as_ints[i] == i);
    REQUIRE(has_all);
}


TEST_CASE("make_palette evenly subsamples")
{
    size_t num_samples = 10'000;
    double sample_distance = rgb::num_colors / static_cast<double>(num_samples - 1);

    auto small = static_cast<unsigned>(std::floor(sample_distance));
    auto large = static_cast<unsigned>(std::ceil(sample_distance));

    auto palette = make_palette(num_samples);
    bool evenly_spaced = true;

    for (size_t i = 0; i < palette.size() - 1; i++)
    {
        auto this_hilbert = hilbert_encode(palette[i]);
        auto next_hilbert = hilbert_encode(palette[i+1]);
        auto distance = next_hilbert - this_hilbert;
        evenly_spaced = evenly_spaced and (distance == small or distance == large);
    }

    REQUIRE(evenly_spaced);
}


TEST_CASE("has_all_colors works")
{
    auto palette = make_palette(rgb::num_colors);
    REQUIRE(has_all_colors(palette));

    palette[0].r = (palette[0].r + 1) % 256;
    REQUIRE(not has_all_colors(palette));
}


TEST_CASE("grid_graph produces a spanning tree")
{
    constexpr int rows = 1000;
    constexpr int cols = 1000;
    grid_graph::rng_type rng{};
    grid_graph g{rows, cols, rng};

    // If a depth-first search visits all nodes once and only once, we have a
    // spanning tree.
    auto order = g.dfs();
    REQUIRE(order.size() == rows * cols);

    std::bitset<rows * cols> visited;
    for (auto node_idx: order)
    {
        size_t uidx = static_cast<size_t>(node_idx);
        REQUIRE(not visited[uidx]);
        visited.set(uidx);
    }
    REQUIRE(visited.all());
}
