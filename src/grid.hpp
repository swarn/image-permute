/* Representing a 2D grid as a graph.
 *
 * I use a random spanning tree across the pixels to linearize the pixel
 * coordinates while preserving some kind of locality. Because this graph is
 * extremely regular, I took a stab at compacting, optimizing, or maybe just
 * over-complicating a specialized implementation of graphs for grids.
 */

#ifndef GRID_HPP
#define GRID_HPP

#include <cstdint>
#include <vector>

#include "pixels.hpp"


class grid_graph
{
public:
    grid_graph(int rows, int cols);

    // Generate a random spanning tree.
    void span();

    // Return the indices in a preordering, like a depth-first search.
    std::vector<size_t> dfs() const;

    // As above, but the shortest branch is always visited first. The random spanning
    // tree has a few very long branches and many very short branches, so this helps
    // to maintain locality.
    std::vector<size_t> sdfs() const;

    // Return the indices sorted by distance from root, like a breadth-first search.
    std::vector<size_t> bfs() const;

private:
    void link_children();
    void build_jump_table();
    void build_neighbors_table();
    void reset();

    // The grid is a row-major array of bytes.
    int rows, cols;
    std::vector<uint8_t> nodes;

    // Index of the root node
    size_t root;
    uint8_t neighbors[16][12];      // see build_neighbors_table()
    int jump[4];                    // see build_jump_table()
};

#endif
