#include <algorithm>
#include <iostream>

#include "clipp.h"

#include "array2d.hpp"
#include "grid.hpp"
#include "hilbert.hpp"
#include "image.hpp"
#include "pixels.hpp"

using namespace clipp;


int main(int argc, char * argv[])
{
    int rows, cols;
    std::string filename;
    bool check = false;
    enum class order {sdfs, dfs, bfs};
    order traversal = order::sdfs;

    auto cli = (
        integer("rows", rows),
        integer("cols", cols),
        value("output", filename),
        option("-check").set(check) % "check if output is a valid allRGB image",
        "random spanning tree traversal order (default: sdfs)" % (
            option("-sdfs").set(traversal, order::sdfs) % "shortest depth first" |
            option("-dfs").set(traversal, order::dfs) % "depth first" |
            option("-bfs").set(traversal, order::bfs) % "breadth first"
        )
    );

    if (not parse(argc, argv, cli))
    {
        std::cerr << make_man_page(cli, argv[0]);
        return -1;
    }

    // Generate the colors for the output image.
    auto palette = make_palette(rows * cols);
    std::sort(palette.begin(), palette.end(), hilbert_compare);

    // My Hilbert sort always goes from black (0, 0, 0) to blue (0, 0, 255).
    // Randomly rotate and flip the color space, to allow other orderings.
    random_color_transformation transform;
    for (auto & color: palette)
        color = transform(color);

    // Generate a random spanning tree across the output image pixels.
    grid_graph graph(rows, cols);
    graph.span();

    // Order the pixels with a traversal of the spanning tree.
    std::vector<size_t> ordering;
    if (traversal == order::bfs)
        ordering = graph.bfs();
    else if (traversal == order::dfs)
        ordering = graph.dfs();
    else
        ordering = graph.sdfs();

    // Copy the (Hilbert-ordered) pixels to the output, in the tree traversal order.
    auto output = array2d<pixel>(rows, cols);
    for (size_t i = 0; i < palette.size(); i++)
        output.data[ordering[i]] = pixel(palette[i]);

    if (check)
    {
        if (has_all_colors(output))
            std::cout << "Has all 2^24 RGB colors\n";
        else
            std::cout << "Not one of each RGB color\n";
    }

    write_image(output, filename.c_str());
    return 0;
}
