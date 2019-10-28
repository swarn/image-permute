// Data types and conversion functions for colors.
//
// Use RGB to determine the color palette and for color blending, use CIELAB
// to measure perceived similarity of color.

#ifndef PIXELS_HPP
#define PIXELS_HPP

#include <iosfwd>
#include <vector>


// Forward declarations so all the types can convert from each other.
struct rgb;
struct rgb_float;
struct xyz;
struct lab;
struct pixel;


struct rgb
{
    rgb() noexcept = default;
    explicit rgb(int) noexcept;             // 0x00000000 to 0x00FFFFFF
    explicit rgb(lab const &) noexcept;
    explicit rgb(xyz const &) noexcept;

    rgb_float operator*(float) const noexcept;
    explicit operator int() const noexcept;

    uint8_t r, g, b;
};

std::ostream & operator<<(std::ostream & out, rgb const & color);


// RGB as float, used in intermediate computation steps.
struct rgb_float
{
    rgb_float() noexcept = default;
    rgb_float(float r, float g, float b) noexcept;
    explicit rgb_float(rgb const &) noexcept;

    rgb_float operator*(float) const noexcept;
    rgb_float operator+(rgb_float const &) const noexcept;

    float r, g, b;
};


struct lab
{
    lab() = default;
    explicit lab(rgb const &) noexcept;
    explicit lab(rgb_float const&) noexcept;
    explicit lab(xyz const &) noexcept;

    float L, a, b;
};

std::ostream & operator<<(std::ostream & out, lab const & color);


// The CIEXYZ colorspace is used as an intermediate step in conversions
// between RGB and LAB.
struct xyz
{
    xyz() = default;
    explicit xyz(rgb const &) noexcept;
    explicit xyz(rgb_float const &) noexcept;
    explicit xyz(lab const &) noexcept;

    float x, y, z;
};


// Store colors in both colorspaces:
// 1. It uses extra memory, but it saves repeated conversions.
// 2. The RGB -> LAB -> RGB conversion doesn't have perfect fidelity, so keep
//    the original RGB values around to make sure we don't alter the palette.
struct pixel
{
    pixel() = default;
    explicit pixel(rgb const &) noexcept;

    rgb rgb;
    lab lab;
};


// Return the squared difference in LAB space.
float diff2(lab const & lhs, lab const & rhs);
float diff2(pixel const & lhs, pixel const & rhs);


// Get a number of RGB colors evenly spread through the RGB colorspace. If
// palette_size is 2^24, you'll get one of each color; otherwise there will be
// gaps or repetitions.
std::vector<rgb> make_palette(int palette_size);


// A mapping to a randomly-oriented color cube.
//
// If you think of the RGB as coordinate tuples, there are 48 ways to interpret
// them: the origin can be in 8 corners, with 6 possible axis configurations at
// each. This class chooses a random interpretation.
struct random_color_transformation
{
    random_color_transformation();

    // Translate from the original color cube to the new one.
    rgb operator()(rgb color) const noexcept;

    int axis_order[3] = {0, 1, 2};
    int axis_inverted[3];
};

#endif
