// Order RGB tuples along a 3D Hilbert curve
//
// There are many possible ways to linearize the 3D RGB colorspace; Hilbert
// curves are a nice one. There are many possible Hilbert curves in 3D spaces;
// the one I implement here is optimal for some locality measures.
//

#ifndef HILBERT_HPP
#define HILBERT_HPP

#include "colors.hpp"


// Return the index of the color along the Hilbert traversal of the RGB
// color space, in [0, 2^24);
unsigned hilbert_encode(rgb c);

// Return the color at the given index of the Hilbert traversal of the RGB
// color space.
rgb hilbert_decode(unsigned d);

// Return true if lhs appears before rhs on a Hilbert traversal of the RGB
// colorspace.
bool hilbert_compare(rgb lhs, rgb rhs);

#endif
