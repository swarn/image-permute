#include "hilbert.hpp"

#include <array>

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


 * This is curve A26.2b.b3 from "An inventory of three-dimensional Hilbert
 * space-filling curves" by Herman Haverkort. It has good locality properties.
 *
 * There are various algorithms to directly convert a coordinate tuple to a
 * Hilbert index and vice-versa, using Gray codes and bitwise operations.
 * They are likely more efficient than the algorithm I implement here, which
 * I came up with by looking at a picture of a Hilbert curve. However, I don't
 * think they implement *this* Hilbert curve.
 */
namespace {

// This is the order that the curve visits each octant.
constexpr std::array<unsigned, 8> octant_for_order = {0, 2, 6, 4, 5, 7, 3, 1};


// Given an octant, what order is it visited among all the octants?
constexpr std::array<unsigned, 8> order_for_octant = {0, 7, 1, 6, 3, 4, 2, 5};


// Transform the RGB coordinates as you subdivide the cube. In a given octant,
// rotate and reflect the RGB coordinates so that, at the next subdivision, the
// octant ordering appears unchanged.
void rotate(unsigned octant, rgb & color)
{
    uint8_t t{};
    switch (octant) {
        case 0:
            t = color.r;
            color.r = color.b;
            color.b = color.g;
            color.g = t;
            break;
        case 1:
            t = color.b;
            color.b = ~color.g;
            color.g = ~t;
            break;
        case 2:
        case 6:
            t = color.g;
            color.g = color.b;
            color.b = color.r;
            color.r = t;
            break;
        case 3:
        case 7:
            t = color.g;
            color.g = ~color.b;
            color.b = ~color.r;
            color.r = t;
            break;
        case 4:
        case 5:
            t = color.g;
            color.g = ~color.r;
            color.r = ~t;
            break;
        default:
            // Should never be here.
            break;
    }
}


// The inverse operation of `rotate`.
void irotate(unsigned octant, rgb & color)
{
    uint8_t t{};
    switch (octant) {
        case 0:
            t = color.g;
            color.g = color.b;
            color.b = color.r;
            color.r = t;
            break;
        case 1:
            t = color.b;
            color.b = ~color.g;
            color.g = ~t;
            break;
        case 2:
        case 6:
            t = color.r;
            color.r = color.b;
            color.b = color.g;
            color.g = t;
            break;
        case 3:
        case 7:
            t = color.g;
            color.g = color.r;
            color.r = ~color.b;
            color.b = ~t;
            break;
        case 4:
        case 5:
            t = color.g;
            color.g = ~color.r;
            color.r = ~t;
            break;
        default:
            // Should never be here.
            break;
    }
}


// What octant is the RGB coordinate in at the given step? Step 0 is the
// top-level division into octants; step 1 is the next division, etc. One
// bit from each channel acts as a coordinate, so at step 0, the RGB tuple
// (0, 0, 128) is in octave 1. At step 1, it's in octant 0.
constexpr uint8_t get_octant(rgb const & c, unsigned step)
{
    uint8_t mask = 0b1000'0000U >> step;
    uint8_t octant = 0;
    octant |= (c.r & mask) != 0 ? 0b100U : 0;
    octant |= (c.g & mask) != 0 ? 0b010U : 0;
    octant |= (c.b & mask) != 0 ? 0b001U : 0;
    return octant;
}

} // anonymous namespace


unsigned hilbert_encode(rgb c)
{
    unsigned retval = 0;

    // Build the index three bits at at time, by finding the order of the
    // top-level octave, then the order of the octave in the first division,
    // etc.
    for (unsigned step = 0; step < 8; step++)
    {
        auto octant = get_octant(c, step);
        retval += order_for_octant[octant] << (7U - step) * 3U;
        rotate(octant, c);
    }

    return retval;
}


rgb hilbert_decode(unsigned d)
{
    rgb retval(0);

    // Invert the process of `hilbert_encode`.
    for (int i = 0; i < 8; i++)
    {
        uint8_t order = d & 0b111U;
        auto octant = octant_for_order[order];

        irotate(octant, retval);

        retval.r >>= 1U;
        retval.g >>= 1U;
        retval.b >>= 1U;

        constexpr uint8_t msb = 0b1000'0000U;
        retval.r |= (octant & 0b0100U) != 0 ? msb : 0U;
        retval.g |= (octant & 0b0010U) != 0 ? msb : 0U;
        retval.b |= (octant & 0b0001U) != 0 ? msb : 0U;

        d >>= 3U;
    }

    return retval;
}


// Essentially duplicate `hilbert_encode`, but possibly save work by returning
// early if the most significant bits can determine the ordering.
bool hilbert_compare(rgb lhs, rgb rhs)
{
    for (unsigned order = 0; order < 8; order++)
    {
        auto lhs_octant = get_octant(lhs, order);
        auto rhs_octant = get_octant(rhs, order);

        if (lhs_octant != rhs_octant)
            return order_for_octant[lhs_octant] < order_for_octant[rhs_octant];

        rotate(lhs_octant, lhs);
        rotate(rhs_octant, rhs);
    }

    // All 8 bits are the same, so it's the same color.
    return false;
}
