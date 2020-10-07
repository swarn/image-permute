/* Representing a 2D grid as a graph.
 *
 * I use a random spanning tree across the pixels to linearize the pixel
 * coordinates while preserving some kind of locality. Because this graph is a
 * grid graph, I took a stab at compacting, optimizing, or maybe just
 * over-complicating a specialized implementation.
 */

#ifndef GRID_HPP
#define GRID_HPP

#include <array>
#include <vector>

#include "XoshiroCpp.hpp"


class grid_graph
{
public:
    // Honestly, the `span` member function should just be templated on the RNG
    // type. But then all that code would have to live in the header file.
    using rng_type = XoshiroCpp::Xoshiro256StarStar;

    // Build a random spanning tree for a grid graph with the given dimensions.
    grid_graph(size_t rows, size_t cols, rng_type & rng);

    // Depth-first search. Return the indices of all nodes in a preordering,
    // starting with the root, then recursively visiting any child trees
    // towards up, right, down, and left.
    std::vector<size_t> dfs() const;

    // Shortest depth-first search. Return the indices of all nodes in a
    // preordering, starting with the root, and then recursively visiting the
    // (up to) four children, starting with the child tree with the smallest
    // height.
    std::vector<size_t> sdfs() const;

    // Return the indices sorted by distance from root, like a breadth-first
    // search.
    std::vector<size_t> bfs() const;

private:
    // Store the nodes in a row-major array.
    struct node_t;
    std::vector<node_t> nodes;
    size_t const rows;
    size_t const cols;

    // One node is the root of the tree.
    size_t root;

    // Make the nodes really compact, one byte, so that the graphs fit into
    // cache and process quickly. Hide some bit-twiddling behind a clear
    // interface.
    struct node_t
    {
        // A status bit indicating if the node has been added to spanning tree.
        bool in_tree: 1;

        // A two-bit integer representing a direction pointing towards the
        // parent of the node.
        uint8_t dir: 2;

        // Four bits, one for each direction. These indicate either grid
        // boundaries (during tree creation) or children (during tree
        // traversal).
        uint8_t edges: 4;

        // Get/set one of the edge bits.
        bool get_edge(uint8_t direction) const;
        void set_edge(uint8_t direction);
    };

    // Initialize each node so that its `edges` member has a bit set for each
    // edge of the graph to which it is adjacent.
    void init_node_edges();

    // The jump table translates a move in one step in each direction to an
    // offset in the nodes array.
    std::array<size_t, 4> jump;
    void init_jump_table();

    // Each node has an `edges` member which indexes into a row of this table,
    // which shows directions of connected nodes. E.g., for the top-left node,
    // only right and down are neighbors. These directions are repeated so that
    // you can randomly generate a number from 0 to 11 for any node and get a
    // valid neighbor.
    std::array<std::array<uint8_t, 12>, 16> neighbors;
    void init_neighbors_table();

    // Generate a random spanning tree.
    void span(rng_type & rng);

    // Repurpose the `edges` members of the nodes. Each node points to its
    // parent with `dir`; add links from each node back to its children using
    // `edges`.
    void link_children();
};

#endif
