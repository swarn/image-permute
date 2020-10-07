#include <iostream>
#include <random>
#include <vector>

#include "clipp.h"

#include "array2d.hpp"
#include "image.hpp"
#include "pixels.hpp"
#include "transformations.hpp"

using namespace clipp;


int main(int argc, char * argv[])
{
    std::string input_name;
    std::string output_name;
    std::string palette_out;
    bool ascending = false;
    int swap_passes = 0;
    int dither_passes = 0;

    auto cli = (
        value("input", input_name),
        value("output", output_name),
        (option("-p") & value("file", palette_out)) % "dump palette to image",
        option("-a").set(ascending)
            .doc("match pixels in ascending order of luminance, without "
                 "regard for hue"),
        (option("-s") & integer("passes", swap_passes))
            .doc("check if swapping pixels looks more like the input"),
        (option("-d") & integer("passes", dither_passes))
            .doc("as above, but check if swapping makes a nine-pixel "
                 "neighborhood look more like the input")
    );

    if (not parse(argc, argv, cli))
    {
        std::cerr << make_man_page(cli, argv[0]);
        return -1;
    }

    auto const input = load_image(input_name.c_str());
    array2d<pixel> output(input.rows, input.cols);
    auto palette = make_palette(input.size());
    for (size_t i = 0; i < output.size(); i++)
        output.data[i] = pixel{palette[i]};

    if (palette_out != "")
        write_image(output, palette_out.c_str());

    std::random_device r;
    std::mt19937_64 gen{r()};
    std::shuffle(output.data.begin(), output.data.end(), gen);

    if (ascending)
    {
        auto sorted = match_ascending(input, output);
        output.data = sorted.data;
    }
    compare_and_swap(input, output, swap_passes);
    compare_and_swap_dithered(input, output, dither_passes);

    write_image(output, output_name.c_str());
    return 0;
}

