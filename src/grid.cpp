#include "grid.hpp"

#include <array>
#include <deque>
#include <queue>
#include <random>
#include <stack>


namespace {

// Enumerate the neighbor directions.
constexpr uint8_t up = 0;
constexpr uint8_t right = 1;
constexpr uint8_t down = 2;
constexpr uint8_t left = 3;

constexpr std::array<uint8_t, 4> directions {up, right, down, left};

constexpr uint8_t opposite_of(uint8_t dir)
{
    return (dir + 2) % 4;
}


constexpr bool has_bit_set(uint8_t n, uint8_t b)
{
    return (n & (1U << b)) != 0;
}

} // anonymous namespace


bool grid_graph::node_t::get_edge(uint8_t direction) const
{
    return has_bit_set(edges, direction);
}


void grid_graph::node_t::set_edge(uint8_t direction)
{
    edges |= (1U << direction);
}


grid_graph::grid_graph(size_t rows, size_t cols, rng_type & rng)
    : nodes {rows * cols, {false, 0, 0}}
    , rows {rows}
    , cols {cols}
    , root {0}
    , jump {}
    , neighbors {}
{
    init_node_edges();
    init_jump_table();
    init_neighbors_table();
    span(rng);
    link_children();
}


void grid_graph::init_node_edges()
{
    // Set a bit in each node along the edges of the grid. Nodes in corners
    // will have two bits set.
    //
    // This is purely an optimization, so that later as we random walk around
    // the grid, we don't have to check for the edge at each step.
    for (size_t i = 0; i < cols; i++)
        nodes[i].set_edge(up);
    for (size_t i = cols * (rows - 1); i < rows * cols; i++)
        nodes[i].set_edge(down);
    for (size_t i = 0; i < rows * cols; i += cols)
        nodes[i].set_edge(left);
    for (size_t i = cols - 1; i < rows * cols; i += cols)
        nodes[i].set_edge(right);
}


void grid_graph::init_jump_table()
{
    // Warning: unsigned arithmetic! The jump table is used e.g.
    //
    //    size_t node_above_n = n + jump[up];
    //
    // Unsigned arithmetic is notoriously error-prone, but the STL containers
    // use unsigned integers everywhere, so the other options are 1) disable
    // signedness warnings, 2) use signed indices and litter the code with
    // casts (also error-prone), or 3) reimplement the STL containers.
    jump[up] = -cols;
    jump[right] = +1;
    jump[down] = +cols;
    jump[left] = -1UL;
}


void grid_graph::init_neighbors_table()
{
    // For each possible value of a nodes `edges` member...
    std::vector<uint8_t> available;
    for (uint8_t edge_value = 0; edge_value < 15; edge_value++)
    {
        // ...build a list of available neighbors. There will be at most four.
        available.clear();
        for (auto const dir: directions)
            if (not has_bit_set(edge_value, dir))
                available.push_back(dir);

        // Repeat these options to fill out twelve possible options (with two,
        // three, or four directions, repeated as needed) for each node.
        for (unsigned choice = 0; choice < 12; choice++)
            neighbors[edge_value][choice] = available[choice % available.size()];
    }

    // Skip edge_value 15, because for that to be true, all four edge bits
    // would have to be set, which only happens in an image with one pixel.
}


void grid_graph::span(rng_type & rng)
{
    std::uniform_int_distribution<size_t> d12(0, 11);
    std::uniform_int_distribution<size_t> rnd_node(0, nodes.size() - 1);

    // Make a random node the root of the tree; mark it as in the tree.
    root = rnd_node(rng);
    nodes[root].in_tree = true;

    // For each node, connect it to the existing tree with a random walk.
    // Strangely enough, the order which you add nodes to the tree doesn't
    // affect the random properties of the final tree!
    for (size_t start = 0; start < nodes.size(); start++)
    {
        // Do a random walk until you connect to the existing tree, recording
        // the direction to the next node (the parent) as you go. Note that the
        // random walk will move in cycles sometimes, overwriting previous
        // parent info: that's OK.
        auto here = start;
        while (not nodes[here].in_tree)
        {
            uint8_t possible = nodes[here].edges;
            uint8_t parent_dir = neighbors[possible][d12(rng)];
            nodes[here].dir = parent_dir;
            here += jump[parent_dir];
        }

        //  After a walk connects to the tree, go back and trace from the first
        //  node to the tree, following the parent directions, marking these
        //  nodes as now in the tree. Any loops/cycles are effectively removed
        //  in this step, because only the final parent direction is followed.
        here = start;
        while (not nodes[here].in_tree)
        {
            nodes[here].in_tree = true;
            here += jump[nodes[here].dir];
        }
    }

    link_children();
}


// After the spanning tree generation in span(), each node points towards its
// parent. During tree traversal, we need to go from a node to its children.
// During traversal, we could simply check each neighbor to see if it points to
// the node in question, but instead we'll do another questionable
// optimization!
//
// Now that the random walks are done, we'll reuse the edge bits to indicate if
// a neighbor is a child or not. Iterate over all the nodes, setting the child
// bit in one neighbor based on the parent direction in the node.
void grid_graph::link_children()
{
    for (auto & node: nodes)
        node.edges = 0;

    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (i == root)
            continue;

        auto parent_dir = nodes[i].dir;
        auto child_dir = opposite_of(parent_dir);
        auto parent = i + jump[parent_dir];
        nodes[parent].set_edge(child_dir);
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

        for (auto const dir: directions)
            if (nodes[idx].get_edge(dir))
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
        for (auto const dir: directions)
            if (nodes[idx].get_edge(dir))
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
        std::array<size_t, 4> children {};
        size_t num_children = 0;
        for (auto const dir: directions)
            if (nodes[node_idx].get_edge(dir))
                children[num_children++] = node_idx + jump[dir];

        // Sort the children in descending order of height, then put the
        // shortest on the stack last, so that the search visits it first.
        auto taller = [&](auto lhs, auto rhs) { return heights[lhs] > heights[rhs]; };
        std::sort(children.begin(), children.begin() + num_children, taller);
        for (size_t i = 0; i < num_children; i++)
            dfs_stack.push(children[i]);
    }

    return order;
}


std::vector<size_t> grid_graph::bfs() const
{
    std::vector<size_t> order;
    std::deque<size_t> unprocessed;
    unprocessed.push_back(root);

    while (not unprocessed.empty())
    {
        auto idx = unprocessed.front();
        unprocessed.pop_front();
        order.push_back(idx);

        for (auto const dir: directions)
            if (nodes[idx].get_edge(dir))
                unprocessed.push_back(idx + jump[dir]);
    }

    return order;
}
