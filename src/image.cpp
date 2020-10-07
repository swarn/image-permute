#include <vector>

#include <png.h>

#include "array2d.hpp"
#include "colors.hpp"


array2d<rgb> load_image(char const * filename)
{
    png_image image;
    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    auto is_png = png_image_begin_read_from_file(&image, filename);
    if (is_png == 0)
        throw std::runtime_error("not a PNG image");

    image.format = PNG_FORMAT_RGB;
    std::vector<png_byte> buffer(PNG_IMAGE_SIZE(image)); // NOLINT

    auto finished = png_image_finish_read(
        &image,
        nullptr,    // background
        buffer.data(),
        0,          // row stride
        nullptr
    );

    if (finished == 0)
        throw std::runtime_error("failed to read PNG image");

    auto iter = buffer.begin();
    array2d<rgb> retval(image.height, image.width);
    for (auto & pixel: retval.data)
    {
        pixel.r = *iter++;
        pixel.g = *iter++;
        pixel.b = *iter++;
    }

    return retval;
}


void write_image(array2d<rgb> const & pixels, char const * filename)
{
    png_image image;
    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(pixels.cols);
    image.height = static_cast<png_uint_32>(pixels.rows);
    image.format = PNG_FORMAT_RGB;

    std::vector<png_byte> buffer(PNG_IMAGE_SIZE(image)); // NOLINT

    auto iter = buffer.begin();
    for (auto const & pixel: pixels.data)
    {
        *iter++ = pixel.r;
        *iter++ = pixel.g;
        *iter++ = pixel.b;
    }

    auto write_complete = png_image_write_to_file(
        &image,
        filename,
        0,          // no need to convert to 8-bit
        buffer.data(),
        0,          // automatically determine row stride
        nullptr     // no colormap
    );

    if (write_complete == 0)
        throw std::runtime_error(image.message); // NOLINT
}

