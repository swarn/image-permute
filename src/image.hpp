// PNG image reading via libPNG
//
// "Quick and dirty" doesn't even begin to describe this code.

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vector>

#include "array2d.hpp"
#include "pixels.hpp"


array2d<pixel> load_image(char const * filename);
void write_image(array2d<pixel> const & pixels, char const * filename);
bool has_all_colors(array2d<pixel> const & pixels);

#endif
