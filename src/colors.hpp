// Color types, conversions between them, and operations producing and
// transforming them.

#ifndef COLORS_HPP
#define COLORS_HPP

#include <array>
#include <vector>

#include "XoshiroCpp.hpp"


struct rgb
{
    static constexpr int num_colors = 256 * 256 * 256;

    rgb() noexcept = default;

    // To and from a web hex code, e.g. 0xFF0000 is red.
    explicit rgb(unsigned) noexcept;
    explicit operator unsigned() const noexcept;

    bool operator==(rgb const &) const;
    bool operator!=(rgb const &) const;

    uint8_t r, g, b;
};

std::ostream & operator<<(std::ostream & out, rgb const & color);


// Get a number of RGB colors evenly spread through the RGB colorspace. If
// `palette_size` is 2^24, you'll get one of each color; otherwise there will
// be gaps or repetitions.
std::vector<rgb> make_palette(size_t palette_size);


// Check if all 2^24 RGB colors are present once, and only once.
bool has_all_colors(std::vector<rgb> const & pixels);


// Forward declaration; `xyz` and `rgb_float` are mutually dependent.
struct xyz;


// RGB represented with floats, used in intermediate computation steps.
//
// Note that I never need to convert from `rgb_float` back to `rgb`. That's
// convenient, because 1) the round-trip conversion isn't perfect and 2) I'd
// have to figure out the correct way to quantize.
struct rgb_float
{
    rgb_float() noexcept = default;

    rgb_float(float r, float g, float b) noexcept : r {r}, g {g}, b {b}
    { }

    explicit rgb_float(rgb const & c) noexcept
        : r {static_cast<float>(c.r)}
        , g {static_cast<float>(c.g)}
        , b {static_cast<float>(c.b)}
    { }

    explicit rgb_float(xyz const &) noexcept;

    rgb_float operator+(rgb_float const & rhs) const
    {
        return rgb_float(r + rhs.r, g + rhs.g, b + rhs.b);
    }

    rgb_float operator-(rgb_float const & rhs) const
    {
        return rgb_float {r - rhs.r, g - rhs.g, b - rhs.b};
    }

    rgb_float operator*(float f) const
    {
        return rgb_float(r * f, g * f, b * f);
    }

    rgb_float & operator+=(rgb_float const & rhs)
    {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }

    float r, g, b;
};


// CIELAB color space, used to measure perceived difference in colors.
struct lab
{
    lab() noexcept = default;

    // Convert CIEXYZ to CIELAB.
    explicit lab(xyz const &) noexcept;

    // Convert sRGB to CIELAB by chaining several conversions.
    explicit lab(rgb const &) noexcept;
    explicit lab(rgb_float const &) noexcept;

    float L, a, b;
};

std::ostream & operator<<(std::ostream & out, lab const & color);

// Return the squared difference in LAB space.
float diff2(lab const & lhs, lab const & rhs);


// The CIEXYZ colorspace is used as an intermediate step in conversions
// between RGB and LAB.
struct xyz
{
    xyz() noexcept = default;
    explicit xyz(rgb_float const &) noexcept;
    explicit xyz(lab const &) noexcept;

    double x, y, z;
};


// A map from one RGB color space to another.
//
// If you think of the RGB values as coordinate triplets, there are 48 ways to
// interpret them: the origin can be in 8 corners, with 6 possible axis
// configurations at each.
class color_transform
{
public:
    using rng_type = XoshiroCpp::Xoshiro256StarStar;

    // Create a randomly-generated `color_transform`.
    static color_transform make_random(rng_type & rng);

    // Translate from the original color cube to the new one.
    rgb operator()(rgb color) const noexcept;

private:
    std::array<uint8_t, 3> axis_order = {0, 1, 2};
    std::array<bool, 3> axis_inverted = {false, false, false};
};

#endif // COLORS_HPP
