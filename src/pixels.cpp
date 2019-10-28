#include "pixels.hpp"

#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "hilbert.hpp"


rgb::rgb(int value) noexcept
{
    r = value >> 16 & 0xFF;
    g = value >>  8 & 0xFF;
    b = value       & 0xFF;
}

rgb::operator int() const noexcept
{
    return (int(r) << 16) | (int(g) << 8) | int(b);
}

rgb::rgb(lab const & input) noexcept
    : rgb{xyz{input}} { }

rgb::rgb(xyz const & input) noexcept
{
    double x = input.x / 100.000;
    double y = input.y / 100.000;
    double z = input.z / 100.000;

    double r = x *  3.2406 + y * -1.5372 + z * -0.4986;
    double g = x * -0.9689 + y *  1.8758 + z *  0.0415;
    double b = x *  0.0557 + y * -0.2040 + z *  1.0570;

    r = (r > 0.0031308) ? (1.055 * std::pow(r, 1.0 / 2.4) - 0.055) : 12.92 * r;
    g = (g > 0.0031308) ? (1.055 * std::pow(g, 1.0 / 2.4) - 0.055) : 12.92 * g;
    b = (b > 0.0031308) ? (1.055 * std::pow(b, 1.0 / 2.4) - 0.055) : 12.92 * b;

    r = std::round(r * 255);
    g = std::round(g * 255);
    b = std::round(b * 255);

    this->r = static_cast<uint8_t>(r);
    this->g = static_cast<uint8_t>(g);
    this->b = static_cast<uint8_t>(b);
}

rgb_float rgb::operator*(float f) const noexcept
{
    return rgb_float(r * f, g * f, b * f);
}

std::ostream & operator<<(std::ostream & out, rgb const & color)
{
    out << static_cast<int>(color.r) << ", "
        << static_cast<int>(color.g) << ", "
        << static_cast<int>(color.b);
    return out;
}


rgb_float::rgb_float(float r, float g, float b) noexcept
    : r{r}, g{g}, b{b} { }

rgb_float::rgb_float(rgb const& c) noexcept
    : rgb_float{c * 1.0f} { }

rgb_float rgb_float::operator+(rgb_float const & other) const noexcept
{
    return rgb_float(r + other.r, g + other.g, b + other.b);
}

rgb_float rgb_float::operator*(float f) const noexcept
{
    return rgb_float(r * f, g * f, b * f);
}


lab::lab(rgb const & color) noexcept
    : lab{xyz{color}} { }

lab::lab(rgb_float const & color) noexcept
    : lab{xyz{color}} { }

lab::lab(xyz const & color) noexcept
{
    double x = color.x /  95.047;
    double y = color.y / 100.000;
    double z = color.z / 108.883;

    x = (x > 0.008856) ? std::pow(x, 1.0 / 3.0) : (7.787 * x) + 16.0 / 116.0;
    y = (y > 0.008856) ? std::pow(y, 1.0 / 3.0) : (7.787 * y) + 16.0 / 116.0;
    z = (z > 0.008856) ? std::pow(z, 1.0 / 3.0) : (7.787 * z) + 16.0 / 116.0;

    L = (116.0 * y) - 16.0;
    a = 500.0 * (x - y);
    b = 200.0 * (y - z);
}

std::ostream & operator<<(std::ostream & out, lab const & color)
{
    out << color.L << ", " << color.a << ", " << color.b;
    return out;
}


xyz::xyz(rgb const & color) noexcept
    : xyz{rgb_float(color)} { }

xyz::xyz(rgb_float const & color) noexcept
{
    float r = color.r / 255.0;
    float g = color.g / 255.0;
    float b = color.b / 255.0;

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
    y = (input.L + 16.0) / 116.0;
    x = input.a / 500.0 + y;
    z = y - input.b / 200.0;

    double x3 = x * x * x;
    double y3 = y * y * y;
    double z3 = z * z * z;
    x = ((x3 > 0.008856) ? x3 : (x - 16.0 / 116.0) / 7.787);
    y = ((y3 > 0.008856) ? y3 : (y - 16.0 / 116.0) / 7.787);
    z = ((z3 > 0.008856) ? z3 : (z - 16.0 / 116.0) / 7.787);

    // Using D65 2° reference
    x *=  95.047;
    y *= 100.000;
    z *= 108.883;
}


pixel::pixel(struct rgb const & color) noexcept
    : rgb{color}, lab{color} { }


std::vector<rgb> make_palette(int palette_size)
{
    constexpr int num_colors = 256 * 256 * 256;

    std::vector<rgb> colors;
    colors.reserve(num_colors);
    for (int i = 0; i < num_colors; i++)
        colors.push_back(rgb(i));

    if (palette_size == num_colors)
        return colors;

    std::sort(colors.begin(), colors.end(), hilbert_compare);
    double const delta = static_cast<double>(num_colors) / (palette_size - 1);

    std::vector<rgb> retval;
    retval.reserve(palette_size);
    for (int i = 0; i < palette_size - 1; i++)
        retval.push_back(colors[i * delta]);

    retval.push_back(colors[num_colors - 1]);

    return retval;
}


float diff2(lab const & lhs, lab const & rhs)
{
    float const dL = lhs.L - rhs.L;
    float const da = lhs.a - rhs.a;
    float const db = lhs.b - rhs.b;
    return dL * dL + da * da + db * db;
}

float diff2(pixel const & lhs, pixel const & rhs)
{
    return diff2(lhs.lab, rhs.lab);
}


random_color_transformation::random_color_transformation()
{
    std::random_device r;
    std::mt19937_64 gen{r()};
    std::shuffle(axis_order, axis_order + 3, gen);
    std::uniform_int_distribution flip(0, 1);
    for (int i = 0; i < 3; i++)
        axis_inverted[i] = flip(gen);
}


rgb random_color_transformation::operator()(rgb color) const noexcept
{
    uint8_t colors[3][2] = {
        {color.r, static_cast<uint8_t>(255 - color.r)},
        {color.g, static_cast<uint8_t>(255 - color.g)},
        {color.b, static_cast<uint8_t>(255 - color.b)}
    };

    rgb changed;
    changed.r = colors[axis_order[0]][axis_inverted[0]];
    changed.g = colors[axis_order[1]][axis_inverted[1]];
    changed.b = colors[axis_order[2]][axis_inverted[2]];
    return changed;
}
