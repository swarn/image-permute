// PNG image reading via libPNG
//
// This is the simplest possible wrapper around libPNG's C API. Error-handling
// is an afterthought. Here there be dragons.

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "array2d.hpp"
#include "colors.hpp"


array2d<rgb> load_image(char const * filename);

void write_image(array2d<rgb> const & pixels, char const * filename);


#endif
