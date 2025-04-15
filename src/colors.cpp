#include "colors.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "hilbert.hpp"


rgb::rgb(unsigned value) noexcept
    : r {static_cast<uint8_t>(value >> 16U)}
    , g {static_cast<uint8_t>(value >> 8U)}
    , b {static_cast<uint8_t>(value)}
{ }


rgb::operator unsigned() const noexcept
{
    return static_cast<unsigned>(r) << 16U | static_cast<unsigned>(g) << 8U | b;
}


bool rgb::operator==(rgb const & rhs) const
{
    return r == rhs.r and g == rhs.g and b == rhs.b;
}


bool rgb::operator!=(rgb const & rhs) const
{
    return not (*this == rhs);
}


std::ostream & operator<<(std::ostream & out, rgb const & color)
{
    out << static_cast<int>(color.r) << ", ";
    out << static_cast<int>(color.g) << ", ";
    out << static_cast<int>(color.b);
    return out;
}


std::vector<rgb> make_palette(size_t palette_size)
{
    std::vector<rgb> colors;
    colors.reserve(palette_size);

    // If we want a palette with one of each color, then it's fastest to simply
    // count through them.
    if (palette_size == rgb::num_colors)
    {
        for (int i = 0; i < rgb::num_colors; i++)
            colors.emplace_back(i);

        return colors;
    }

    // If we want to subsample or oversample the color space, that's a bit
    // trickier.
    //
    // We want to avoid aliasing. The `colors` as generated above count up with
    // the blue channel as the least-significant digits, so taking every n
    // colors will tend to simply quantize the blue channel, leaving the others
    // unchanged.
    //
    // Also, we want to guarantee some notion of "evenly spaced." Simply
    // randomly sampling the colors could lead to some colors being randomly
    // overrepresented. Honestly, this is probably not a real issue.
    //
    // The best way to do this might be to generate 3D blue noise in the color
    // space for `(palette_size % rgb::num_colors)` samples. That would be
    // evenly-spaced and not aliased, but it also sounds like a lot of work.
    //
    // Instead, we'll evenly sample along the Hilbert curve through the color
    // space. That will at least guarantee that we're not always slicing along
    // one axis of the space.
    double const delta = rgb::num_colors / static_cast<double>(palette_size - 1);
    for (size_t i = 0; i < palette_size - 1; i++)
    {
        auto hilbert_idx = static_cast<unsigned>(static_cast<double>(i) * delta);
        colors.push_back(hilbert_decode(hilbert_idx));
    }

    // The math works such that, for the last sample, we just manually select
    // the last color. This means that `palette_size` must be at least two, which
    // would return the first and last colors.
    colors.push_back(hilbert_decode(rgb::num_colors - 1));

    return colors;
}


bool has_all_colors(std::vector<rgb> const & pixels)
{
    if (pixels.size() != rgb::num_colors)
        return false;

    std::bitset<rgb::num_colors> has_color;

    for (auto const & color: pixels)
        has_color[unsigned(color)] = true;

    return has_color.all();
}


rgb_float::rgb_float(xyz const & input) noexcept
{
    double const x = input.x / 100.000;
    double const y = input.y / 100.000;
    double const z = input.z / 100.000;

    auto rc = (x * +3.2406) + (y * -1.5372) + (z * -0.4986);
    auto gc = (x * -0.9689) + (y * +1.8758) + (z * +0.0415);
    auto bc = (x * +0.0557) + (y * -0.2040) + (z * +1.0570);

    r = static_cast<float>(
        (rc > 0.0031308) ? ((1.055 * std::pow(rc, 1.0 / 2.4)) - 0.055) : 12.92 * rc);
    g = static_cast<float>(
        (gc > 0.0031308) ? ((1.055 * std::pow(gc, 1.0 / 2.4)) - 0.055) : 12.92 * gc);
    b = static_cast<float>(
        (bc > 0.0031308) ? ((1.055 * std::pow(bc, 1.0 / 2.4)) - 0.055) : 12.92 * bc);
}


lab::lab(xyz const & color) noexcept
{
    auto x = color.x / 095.047;
    auto y = color.y / 100.000;
    auto z = color.z / 108.883;

    x = (x > 0.008856) ? std::pow(x, 1.0 / 3.0) : (7.787 * x) + (16.0 / 116.0);
    y = (y > 0.008856) ? std::pow(y, 1.0 / 3.0) : (7.787 * y) + (16.0 / 116.0);
    z = (z > 0.008856) ? std::pow(z, 1.0 / 3.0) : (7.787 * z) + (16.0 / 116.0);

    L = static_cast<float>((116.0 * y) - 16.0);
    a = static_cast<float>(500.0 * (x - y));
    b = static_cast<float>(200.0 * (y - z));
}


lab::lab(rgb const & color) noexcept : lab {xyz {rgb_float {color}}}
{ }


lab::lab(rgb_float const & color) noexcept : lab {xyz {color}}
{ }


std::ostream & operator<<(std::ostream & out, lab const & color)
{
    out << color.L << ", " << color.a << ", " << color.b;
    return out;
}


xyz::xyz(rgb_float const & color) noexcept
{
    auto r = static_cast<double>(color.r) / 255.0;
    auto g = static_cast<double>(color.g) / 255.0;
    auto b = static_cast<double>(color.b) / 255.0;

    r = (r > 0.04045) ? std::pow((r + 0.055) / 1.055, 2.4) : (r / 12.92);
    g = (g > 0.04045) ? std::pow((g + 0.055) / 1.055, 2.4) : (g / 12.92);
    b = (b > 0.04045) ? std::pow((b + 0.055) / 1.055, 2.4) : (b / 12.92);

    r *= 100;
    g *= 100;
    b *= 100;

    x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    z = r * 0.0193 + g * 0.1192 + b * 0.9505;
}


xyz::xyz(lab const & input) noexcept
{
    auto const L = static_cast<double>(input.L);
    auto const a = static_cast<double>(input.a);
    auto const b = static_cast<double>(input.b);

    y = (L + 16.0) / 116.0;
    x = a / 500.0 + y;
    z = y - b / 200.0;

    double const x3 = x * x * x;
    double const y3 = y * y * y;
    double const z3 = z * z * z;
    x = ((x3 > 0.008856) ? x3 : (x - 16.0 / 116.0) / 7.787);
    y = ((y3 > 0.008856) ? y3 : (y - 16.0 / 116.0) / 7.787);
    z = ((z3 > 0.008856) ? z3 : (z - 16.0 / 116.0) / 7.787);

    // Using D65 2° reference
    x *= 095.047;
    y *= 100.000;
    z *= 108.883;
}


float diff2(lab const & lhs, lab const & rhs)
{
    auto dL = lhs.L - rhs.L;
    auto da = lhs.a - rhs.a;
    auto db = lhs.b - rhs.b;
    return (dL * dL) + (da * da) + (db * db);
}


color_transform color_transform::make_random(rng_type & rng)
{
    color_transform transform;

    std::shuffle(transform.axis_order.begin(), transform.axis_order.end(), rng);

    std::uniform_int_distribution<int> flip(0, 1);
    for (auto & inversion: transform.axis_inverted)
        inversion = (flip(rng) != 0);

    return transform;
}


rgb color_transform::operator()(rgb color) const noexcept
{
    std::array<uint8_t, 3> colors {
        axis_inverted[0] ? static_cast<uint8_t>(~color.r) : color.r,
        axis_inverted[1] ? static_cast<uint8_t>(~color.g) : color.g,
        axis_inverted[2] ? static_cast<uint8_t>(~color.b) : color.b};

    rgb changed {};
    changed.r = colors[axis_order[0]];
    changed.g = colors[axis_order[1]];
    changed.b = colors[axis_order[2]];
    return changed;
}
