// Order RGB tuples along a 3D Hilbert curve
//
// There are many possible ways to linearize the 3D RGB colorspace; Hilbert
// curves are a nice one.
//
// There are various algorithms to directly convert a coordinate tuple to a
// Hilbert index and vice-versa, using Gray codes and bitwise operations.
// They are likely more efficient than the algorithm I implement here, which
// I came up with by looking at a picture of a Hilbert curve.
//
// Rather than translate between coordinates and Hilber indices, this simply
// tells you which coordinate (that is, RGB color) occurs before the other on
// the Hilbert curve.

#ifndef HILBERT_HPP
#define HILBERT_HPP

#include "pixels.hpp"


// Return true if lhs appears before rhs on a Hilbert traversal of the RGB
// colorspace.
bool hilbert_compare(rgb lhs, rgb rhs);

#endif
