#include <bitset>

#include <png.h>

#include "array2d.hpp"
#include "pixels.hpp"


array2d<pixel> load_image(char const * filename)
{
    png_image image;
    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    auto is_png = png_image_begin_read_from_file(&image, filename);
    if (not is_png)
        throw std::runtime_error("not a PNG image");

    png_bytep buffer;
    image.format = PNG_FORMAT_RGB;
    buffer = static_cast<png_bytep>(malloc(PNG_IMAGE_SIZE(image)));
    if (buffer == nullptr)
        throw std::runtime_error("failed to allocate read buffer");

    auto finished = png_image_finish_read(
        &image,
        nullptr,    // background
        buffer,
        0,          // row stride
        nullptr
    );

    if (not finished)
        throw std::runtime_error("failed to read PNG image");

    auto iter = buffer;
    array2d<pixel> retval(image.height, image.width);
    for (int row = 0; row < retval.rows; row++)
    {
        for (int col = 0; col < retval.cols; col++)
        {
            rgb color;
            color.r = *iter++;
            color.g = *iter++;
            color.b = *iter++;
            retval(row, col) = pixel(color);
        }
    }

    free(buffer);
    return retval;
}


void write_image(array2d<pixel> const & pixels, char const * filename)
{
    png_image image;
    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;
    image.width = pixels.cols;
    image.height = pixels.rows;
    image.format = PNG_FORMAT_RGB;

    png_bytep buffer;
    buffer = static_cast<png_bytep>(malloc(PNG_IMAGE_SIZE(image)));
    if (buffer == nullptr)
        throw std::runtime_error("failed to write buffer");

    auto iter = buffer;
    for (auto const & pixel: pixels.data)
    {
        *iter++ = pixel.rgb.r;
        *iter++ = pixel.rgb.g;
        *iter++ = pixel.rgb.b;
    }

    auto write_complete = png_image_write_to_file(
        &image,
        filename,
        0,          // no need to convert to 8-bit
        buffer,
        0,          // automatically determine row stride
        nullptr     // no colormap
    );

    if (not write_complete)
        throw std::runtime_error(image.message);

    free(buffer);
}


bool has_all_colors(array2d<pixel> const & pixels)
{
    constexpr int num_colors = 256 * 256 * 256;
    if (pixels.size != num_colors)
        return false;

    std::bitset<num_colors> has_color;

    for (auto const & color: pixels.data)
        has_color[int(color.rgb)] = true;

    return has_color.all();
}
