#include "color.h"

color color_to_256(color c, int samples_per_pixel);

void write_color(std::ostream &out, color pixel_color, int samples_per_pixel);
