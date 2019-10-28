#include "hilbert.hpp"

/* This 3D version of the Hilbert curve is (I think) a fractal. Divide a cube
 * into octants and visit them in some order. Further divide each octant into
 * another eight octants, and visit them in the same shape, with some
 * reflection or translation.
 *
 * The drawing below shows the octant order; start at B in octant 0 and end at
 * E in octant 1. The octants are labeled in binary, to make it easier to see
 * how each bit of the octant number corresponds to an axis of 3D space. The
 * rotation/reflection of the shape in each octant is also shown.


                    *----------*                   *----------*
                    |         /                   /           |
                    |        /                   /            |
                 *----------*                   *----------*  |
                .   |                                     .   |
               .    *----------*                   *-----.----*
              .               /                   /     .
             .        110    /                   /  111.
            .    *----------*                   *-----.----*
           .     .                                   .     .
          *      .   * . . . . . . . . . *          *      .
          |      .   |                   |          |      .
          |      .   |                   |          |      .
       *----------*  |                *----------*  |      .
       |  |      .|  |                |  |       |  |      .
       |  *      .|  *                |  *       |  *      .
       | /       .| /                 | /        | /       .
       |/   100  .|/                  |/   101   |/        .
       *         .*                   *          *         .
                 .                                         .
                 .  *----------*                   *-------.--*
                 .  |         /                   /        .  |
                 .  |        /                   /         .  |
                 *----------*                   *----------*  |
                    |                                         |
                    *----------*                   *----------*
                              /                   /
                      010    /                   /   011
                 *----------*                   *----------*
                .                                         .
          *----------*                   *----------*    .
          |   .      |                   |         /    .
          |  .       |                   |        /    .
       *----------*  |                *----------*    .
       |  |.      |  |                |  |           .
       |  *       |  *                |  *----------*
       |          | /                 |
       |    000   |/                  |    001
       B          *                   *----------E


 * This is curve A26.43.179 from "An inventory of three-dimensional Hilbert
 * space-filling curves" by Herman Haverkort.
 */

namespace {

// The octants are always visited in the same order (after a possible rotation
// and reflection). This is a small lookup table, such that order[a][b] is true
// iff octant a is visited before octant b.
constexpr bool order[8][8] = {
//      000     001     010     011     100     101     110     111
/*000*/ {false, true,   true,   true,   true,   true,   true,   true},
/*001*/ {false, false,  false,  false,  false,  false,  false,  false},
/*010*/ {false, true,   false,  true,   true,   true,   true,   true},
/*011*/ {false, true,   false,  false,  false,  false,  false,  false},
/*100*/ {false, true,   false,  true,   false,  true,   false,  true},
/*101*/ {false, true,   false,  true,   false,  false,  false,  true},
/*110*/ {false, true,   false,  true,   true,   true,   false,  true},
/*111*/ {false, true,   false,  true,   false,  false,  false,  false}
};


// The RGB colorspace is a cube 256 units long on each side. The most
// significant bit of each of the color channels determines which octant of the
// cube contains the given color.
constexpr int octant(rgb const & c)
{
    return ((c.r & 0x80) >> 5) | ((c.g & 0x80) >> 6) | ((c.b & 0x80) >> 7);
}

} // anonymous namespace


// If the colors are in different octants, simply return the order from the
// table above. If they're in the same octant, recursively divide that octant
// into further octants by looking at the next bits of the colors. Each octant
// reflects and/or rotates its contents.
bool hilbert_compare(rgb lhs, rgb rhs)
{
    for (int i = 0; i < 8; i++)
    {
        auto lhs_octant = octant(lhs);
        auto rhs_octant = octant(rhs);

        if (lhs_octant != rhs_octant)
            return order[lhs_octant][rhs_octant];

        lhs.r <<= 1;
        lhs.g <<= 1;
        lhs.b <<= 1;
        rhs.r <<= 1;
        rhs.g <<= 1;
        rhs.b <<= 1;

        uint8_t t;
        switch (lhs_octant)
        {
        case 0:
            t = lhs.r;
            lhs.r = lhs.b;
            lhs.b = lhs.g;
            lhs.g = t;
            t = rhs.r;
            rhs.r = rhs.b;
            rhs.b = rhs.g;
            rhs.g = t;
            break;
        case 1:
            t = lhs.b;
            lhs.b = ~lhs.g;
            lhs.g = ~t;
            t = rhs.b;
            rhs.b = ~rhs.g;
            rhs.g = ~t;
            break;
        case 2:
        case 6:
            t = lhs.g;
            lhs.g = lhs.b;
            lhs.b = lhs.r;
            lhs.r = t;
            t = rhs.g;
            rhs.g = rhs.b;
            rhs.b = rhs.r;
            rhs.r = t;
            break;
        case 3:
        case 7:
            t = lhs.g;
            lhs.g = ~lhs.b;
            lhs.b = ~lhs.r;
            lhs.r = t;
            t = rhs.g;
            rhs.g = ~rhs.b;
            rhs.b = ~rhs.r;
            rhs.r = t;
            break;
        case 4:
        case 5:
            t = lhs.g;
            lhs.g = ~lhs.r;
            lhs.r = ~t;
            t = rhs.g;
            rhs.g = ~rhs.r;
            rhs.r = ~t;
            break;
        }
    }

    // All 8 bits are the same, so it's the same color.
    return false;
}
