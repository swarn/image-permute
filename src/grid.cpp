#include "grid.hpp"

#include <iostream>
#include <deque>
#include <queue>
#include <random>
#include <stack>
#include <vector>

namespace {

// TODO: all the masks should be arrays indexed by direction.

// Enumerate the neighbor directions.
constexpr uint8_t up    = 0;
constexpr uint8_t right = 1;
constexpr uint8_t down  = 2;
constexpr uint8_t left  = 3;

constexpr uint8_t opposite_of(uint8_t dir)
{
    return (dir + 2) % 4;
}

// At each node, set one or more bits indicating whether it is at the edge of
// the grid, so we don't have to check at every step of the random walk. These
// bits are used to index into the neighbors table.
constexpr uint8_t at_edge[4] = {1, 2, 4, 8};
constexpr uint8_t edge_mask = 0b00001111;

// Use one bit to record if the node has been added to the spanning tree.
constexpr uint8_t in_tree = 0b10000000;

// Each node has a parent (except the root), recorded as the direction to the
// parent, and packed into two bits of the node value.
constexpr uint8_t parent_mask = 0b00110000;

constexpr uint8_t get_parent_dir(uint8_t node)
{
    return (node & parent_mask) >> 4;
}

void set_parent_dir(uint8_t & node, uint8_t dir)
{
    node = (node & ~parent_mask) | (dir << 4);
}


// After generating the tree, the bits used to indicate edges/neighbors are
// no longer useful, and we can reuse them to record the children of each node.
constexpr uint8_t child_mask[4] = {1, 2, 4, 8};

} // anonymous namespace


grid_graph::grid_graph(int rows, int cols)
    : rows{rows}, cols{cols}
{
    build_jump_table();
    build_neighbors_table();
    nodes.resize(rows * cols);
}


// To avoid a list of if statements for each step of the random walk, build a
// simple jump table indexed by the direction, with the offset to the index for
// each direction.
void grid_graph::build_jump_table()
{
    jump[up] = -cols;
    jump[right] = +1;
    jump[down] = +cols;
    jump[left] = -1;
}


// The neighbors table is used during the random walk. A naive approach would
// build a list of adjacent pixels before every random step. There are a
// limited number of options: most pixels have four neighbors, those on the top
// row have three neighbors, those on the bottom row have three neighbors in
// different directions, etc. So, we can build a lookup table instead, and
// precompute an index into the table for each node.
//
// Each node has four bits indicating whether it's on one or more edges, for 16
// possible indexes, though only 9 of those will occur in images with more than
// one row and column.
//
// Because pixels might have two, three, or four options, we might store the
// number of possible directions, as well as the directions themselves.
// Instead, I store 12 options for each, repeating the options six, four, or
// three times, and all steps in the walk simply use a random number in [0, 11].
// I haven't measured to see if this is actually a useful optimization.
void grid_graph::build_neighbors_table()
{
    std::vector<uint8_t> directions;
    for (int i = 0; i < 15; i++)
    {
        directions.clear();
        for (int dir = 0; dir < 4; dir++)
            if (not (at_edge[dir] & i))
                directions.push_back(dir);

        for (int j = 0; j < 12; j++)
            neighbors[i][j] = directions[j % directions.size()];
    }
}


// Reset the grid nodes to having the correct entries into the neighbors table,
// and clear their "in-tree" bits.
void grid_graph::reset()
{
    for (auto & node: nodes)
        node = 0;
    for (int i = 0; i < cols; i++)
        nodes[i] |= at_edge[up];
    for (int i = cols * (rows - 1); i < rows * cols; i++)
        nodes[i] |= at_edge[down];
    for (int i = 0; i < rows * cols; i += cols)
        nodes[i] |= at_edge[left];
    for (int i = cols - 1; i < rows * cols; i += cols)
        nodes[i] |= at_edge[right];
}


// 1. Make a random node the root of the tree; mark it as in the tree.
// 2. From each node, do a random walk until you connect to the existing tree,
//    recording the direction to the next node (the parent) as you go. Note
//    that the random walk will create cycles and overwrite previous parent
//    info: that's OK.
// 3. After a walk connects to the tree, go back and trace from the first node
//    to the tree, following the parent directions, and marking these nodes as
//    now in the tree. There are now no cycles.
void grid_graph::span()
{
    reset();

    std::random_device r;
    std::mt19937_64 gen{r()};
    std::uniform_int_distribution<> d12(0, 11);
    std::uniform_int_distribution<> rnd_node(0, nodes.size() - 1);
    root = rnd_node(gen);
    nodes[root] |= in_tree;
    for (size_t start = 0; start < nodes.size(); start++)
    {
        auto here = start;
        while (not (nodes[here] & in_tree))
        {
            uint8_t possible = nodes[here] & edge_mask;
            uint8_t parent_dir = neighbors[possible][d12(gen)];
            set_parent_dir(nodes[here], parent_dir);
            here += jump[parent_dir];
        }

        here = start;
        while (not (nodes[here] & in_tree))
        {
            nodes[here] |= in_tree;
            here += jump[get_parent_dir(nodes[here])];
        }
    }

    link_children();
}


// After the spanning tree generation in span(), each node points towards its
// parent. During tree traversal, we need to go from a node to its children. We
// could simply check each neighbor to see if it points to the node in question,
// but instead we'll do another questionable optimization!
//
// Now that the random walks are done, we'll reuse the edge bits to indicate if
// a neighbor is a child or not. Iterate over all the nodes, setting the child
// bit in one neighbor based on the parent direction in the node.
void grid_graph::link_children()
{
    for (size_t i = 0; i < nodes.size(); i++)
        nodes[i] &= (~edge_mask);

    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (i == root)
            continue;

        auto parent_dir = get_parent_dir(nodes[i]);
        auto child_dir = opposite_of(parent_dir);
        auto parent = i + jump[parent_dir];
        nodes[parent] |= child_mask[child_dir];
    }
}


std::vector<size_t> grid_graph::dfs() const
{
    std::vector<size_t> order;
    std::stack<size_t> unprocessed;
    unprocessed.push(root);

    while (not unprocessed.empty())
    {
        auto idx = unprocessed.top();
        unprocessed.pop();
        order.push_back(idx);

        for (uint8_t dir = 0; dir < 4; dir++)
            if (nodes[idx] & child_mask[dir])
                unprocessed.push(idx + jump[dir]);
    }

    return order;
}


std::vector<size_t> grid_graph::sdfs() const
{
    auto order = dfs();

    // Record the height at every node by working up from the leaves.
    std::vector<int> heights(rows * cols);
    while (not order.empty())
    {
        auto idx = order.back();
        order.pop_back();

        int height = 0;
        for (uint8_t dir = 0; dir < 4; dir++)
            if (nodes[idx] & child_mask[dir])
                height = std::max(height, heights[idx + jump[dir]] + 1);

        heights[idx] = height;
    }

    std::stack<size_t> dfs_stack;
    dfs_stack.push(root);
    while (not dfs_stack.empty())
    {
        auto node_idx = dfs_stack.top();
        dfs_stack.pop();
        order.push_back(node_idx);

        // Build a list of up to four children.
        int children[4];
        int num_children = 0;
        for (int dir = 0; dir < 4; dir++)
            if (nodes[node_idx] & child_mask[dir])
                children[num_children++] = node_idx + jump[dir];

        // Sort the children in descending order of height.
        for (int i = 1; i < num_children; i++)
            for (int j = i; j > 0 && heights[children[j-1]] < heights[children[j]]; j--)
                std::swap(children[j], children[j-1]);

        // Put the smallest tree on the stack last, so we visit it next.
        for (int i = 0; i < num_children; i++)
            dfs_stack.push(children[i]);
    }

    return order;
}


std::vector<size_t> grid_graph::bfs() const
{
    std::vector<size_t> order;
    std::deque<size_t> unprocessed;
    unprocessed.push_back(root);

    while(not unprocessed.empty())
    {
        auto idx = unprocessed.front();
        unprocessed.pop_front();
        order.push_back(idx);

        for (int dir = 0; dir < 4; dir++)
            if (nodes[idx] & child_mask[dir])
                unprocessed.push_back(idx + jump[dir]);
    }

    return order;
}


