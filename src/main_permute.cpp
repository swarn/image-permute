#include <algorithm>
#include <iostream>
#include <random>
#include <string>

#include <clipp.h>

#include "array2d.hpp"
#include "colors.hpp"
#include "image.hpp"
#include "permutations.hpp"

using namespace clipp;


int main(int argc, char * argv[]) // NOLINT(bugprone-exception-escape)
{
    std::string input_name;
    std::string output_name;
    std::string palette_out;
    bool ascending = false;
    int swap_passes = 0;
    int dither_passes = 0;
    unsigned seed = 0;
    bool cli_seed = false;

    clipp::group const cli {
        value("input", input_name),
        value("output", output_name),
        (option("-p") & value("file", palette_out)) % "dump palette to image",
        (option("-a").set(ascending)) %
            "Match pixels in ascending order of luminance, without regard for hue or "
            "saturation.",
        (option("-s") & integer("passes") >> swap_passes) %
            "Swap pixels if it makes them look more like the input image. Passes is "
            "roughly how many times it tries for each pixel",
        (option("-d") & integer("passes") >> dither_passes) %
            "Swap pixels if it makes their neighborhood look more like the input "
            "image, which effects color dithering.",
        (option("-seed").set(cli_seed) & integer("n") >> seed) %
            "set random seed value"};

    if (not parse(argc, argv, cli))
    {
        std::cerr << make_man_page(cli, argv[0]);
        return -1;
    }

    if (not cli_seed)
        seed = std::random_device()();

    auto const input = load_image(input_name.c_str());
    array2d<rgb> output(input.rows, input.cols);
    output.data = make_palette(input.size());

    if (not palette_out.empty())
        write_image(output, palette_out.c_str());

    permute_rng_type rng {seed};
    std::shuffle(output.data.begin(), output.data.end(), rng);

    if (ascending)
        match_ascending(input, output);
    if (swap_passes > 0)
        compare_and_swap(input, output, swap_passes, rng);
    if (dither_passes > 0)
        compare_and_swap_dithered(input, output, dither_passes, rng);

    write_image(output, output_name.c_str());
    return 0;
}
