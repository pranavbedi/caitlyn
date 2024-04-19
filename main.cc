#include <embree4/rtcore.h>
#include "CSRParser.h"
#include "device.h"

#include "CLIParser.h"

#include "output.h"

void box_test() {
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 1200, 5, 5);

    point3 lookfrom(10, 5, 0);
    point3 lookat(0, 2, 0);
    vec3 vup(0,1,0);
    double vfov = 60;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    auto mt = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto box = make_shared<BoxPrimitive>(point3(0, 0, 0), vec3(2, 0, -2), vec3(2, 0, 2), vec3(0, 2, 0), mt, device);
    scene_ptr->add_primitive(box);

    scene_ptr->commitScene();
    rtcReleaseDevice(device);

    Config config;
    output(render_data, cam, scene_ptr, config);
}

int main(int argc, char* argv[]) {
    Config config = parseArguments(argc, argv);
    
    RenderData render_data;
    const auto aspect_ratio = static_cast<float>(config.image_width) / config.image_height;
    setRenderData(render_data, aspect_ratio, config.image_width, config.samples_per_pixel, config.max_depth);
    std::string filePath = config.inputFile;
    RTCDevice device = initializeDevice();
    CSRParser parser;
    auto scene_ptr = parser.parseCSR(filePath, device);
    scene_ptr->commitScene();
    rtcReleaseDevice(device);

    output(render_data, scene_ptr->cam, scene_ptr, config);
}

