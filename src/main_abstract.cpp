#include <algorithm>
#include <iostream>
#include <random>

#include <XoshiroCpp.hpp>
#include <clipp.h>

#include "array2d.hpp"
#include "colors.hpp"
#include "grid.hpp"
#include "hilbert.hpp"
#include "image.hpp"

using namespace clipp;


int main(int argc, char * argv[])
{
    size_t rows = 0;
    size_t cols = 0;
    unsigned seed = 0;
    bool cli_seed = false;
    std::string filename;
    bool check = false;
    enum class order
    {
        sdfs,
        dfs,
        bfs
    };
    order traversal = order::sdfs;

    clipp::group cli {
        integer("rows", rows),
        integer("cols", cols),
        value("output", filename),
        option("-check").set(check) % "check if output is a valid allRGB image",
        group {
            option("-sdfs").set(traversal, order::sdfs) % "shortest depth first" |
            option("-dfs").set(traversal, order::dfs) % "depth first" |
            option("-bfs").set(traversal, order::bfs) % "breadth first"}
            .doc("random spanning tree traversal order (default: sdfs"),
        (option("-seed") & integer("n", seed).set(cli_seed)) % "set random seed value"};

    if (not parse(argc, argv, cli))
    {
        std::cerr << make_man_page(cli, argv[0]);
        return -1;
    }

    if (not cli_seed)
        seed = std::random_device()();

    // Generate the colors for the output image.
    auto palette = make_palette(rows * cols);
    std::sort(palette.begin(), palette.end(), hilbert_compare);

    grid_graph::rng_type rng {seed};

    // My Hilbert sort always goes from black (0, 0, 0) to blue (0, 0, 255).
    // Randomly rotate and flip the color space, to allow other orderings.
    auto transform = color_transform::make_random(rng);
    for (auto & color: palette)
        color = transform(color);

    // Generate a random spanning tree across the output image pixels.
    grid_graph graph(rows, cols, rng);

    // Order the pixels with a traversal of the spanning tree.
    std::vector<size_t> ordering;
    if (traversal == order::bfs)
        ordering = graph.bfs();
    else if (traversal == order::dfs)
        ordering = graph.dfs();
    else
        ordering = graph.sdfs();

    // Copy the (Hilbert-ordered) pixels to the output, in the tree traversal order.
    auto output = array2d<rgb>(rows, cols);
    for (size_t i = 0; i < palette.size(); i++)
        output.data[ordering[i]] = palette[i];

    if (check)
    {
        if (has_all_colors(output.data))
            std::cout << "Has all 2^24 RGB colors\n";
        else
            std::cout << "Not one of each RGB color\n";
    }

    write_image(output, filename.c_str());
    return 0;
}
