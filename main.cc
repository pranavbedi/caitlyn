#include <embree4/rtcore.h>
#include "CSRParser.h"
#include "device.h"
#include "general.h"
#include "scene.h"
#include "camera.h"
#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "material.h"
#include "render.h"

#include "sphere_primitive.h"
#include "quad_primitive.h"
#include "intersects.h"
#include "texture.h"
#include "light.h"

#include "CLIParser.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include <iostream>
#include <chrono>

#include <functional>
#include <fstream>
#include "png_output.h"

// Threading
#include <vector>
#include <thread>

/** @brief Create a standardized scene benchmark for testing optimizations between different versions 
 * 
 * @param shared_ptr<Scene> scene_ptr Pointer to the scene that will be modified.
 * @param RTCDevice device object for instantiation. must not be released yet.
 * @note Benchmark v0.1.0
 * @note Standard benchmark scene creates a large ground sphere with 1000 radius, at 0,-1000,0
 * @note Then instantiate 22*22 sphere. In each iteration, choose randomized position and material.
*/
void setup_benchmark_scene(std::shared_ptr<Scene> scene_ptr, RTCDevice device) {
    std::cerr << "Setup Benchmark Scene v0.1.0" << std::endl;
    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    auto ground_sphere = make_shared<SpherePrimitive>(point3(0,-1000,0), ground_material, 1000, device);
    unsigned int groundID = scene_ptr->add_primitive(ground_sphere);

    for (int a = -11; a < 11; a++) {

        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;
                
                // diffuse material
                if (choose_mat < 0.8) {
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                } 

                // metal
                else if (choose_mat < 0.95) {
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                } 

                // glass
                else { sphere_material = make_shared<dielectric>(1.5); }
                
                auto sphere = make_shared<SpherePrimitive>(center, sphere_material, 0.2, device);
                scene_ptr->add_primitive(sphere);
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    auto sphere1 = make_shared<SpherePrimitive>(point3(0, 1, 0), material1, 1, device);
    scene_ptr->add_primitive(sphere1);

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    auto sphere2 = make_shared<SpherePrimitive>(point3(-4, 1, 0), material2, 1, device);
    scene_ptr->add_primitive(sphere2);

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    auto sphere3 = make_shared<SpherePrimitive>(point3(4, 1, 0), material3, 1, device);
    scene_ptr->add_primitive(sphere3);

    // Finalizing the Scene
    scene_ptr->commitScene();
    std::cerr << "COMMIT SCENE :: complete" << std::endl;
}

/**
 * @brief Given scene, camera, and render_data, output ppm pixels image to std::cout
*/

struct RGB{
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

void output(RenderData& render_data, Camera& cam, std::shared_ptr<Scene> scene_ptr) {
    int image_height = render_data.image_height;
    int image_width = render_data.image_width;
    int samples_per_pixel = render_data.samples_per_pixel;

    // Start Render Timer 
    auto start_time = std::chrono::high_resolution_clock::now();
    render_data.completed_lines = 0;

    // To render entire thing without multithreading, uncomment this line and comment out num_threads -> threads.clear()
    render_scanlines(image_height,image_height-1,scene_ptr,render_data,cam);

    // // Threading approach? : Divide the scanlines into N blocks
    // const int num_threads = std::thread::hardware_concurrency() - 1;

    // // Image height is the number of scanlines, suppose image_height = 800
    // const int lines_per_thread = image_height / num_threads;
    // const int leftOver = image_height % num_threads;
    // // The first <num_threads> threads are dedicated <lines_per_thread> lines, and the last thread is dedicated to <leftOver>

    // std::vector<color> pixel_colors;
    // std::vector<std::thread> threads;

    // render_data.completed_lines = 0;

    // for (int i=0; i < num_threads; i++) {
    //     // In the first thead, we want the first lines_per_thread lines to be rendered
    //     threads.emplace_back(render_scanlines_sse,lines_per_thread,(image_height-1) - (i * lines_per_thread), scene_ptr, std::ref(render_data),cam);
    // }
    // threads.emplace_back(render_scanlines_sse,leftOver,(image_height-1) - (num_threads * lines_per_thread), scene_ptr, std::ref(render_data),cam);

    // for (auto &thread : threads) {
    //         thread.join();
    // }
    // std::cerr << "Joining all threads" << std::endl;
    // threads.clear();

    int output_type = 2; // 0 for ppm, 1 for jpg, 2 for png
    // hardcoded, but will be updated for CLI in CA-83

    if (output_type == 0) {
        std::cout << "P3" << std::endl;
        std::cout << image_width << ' ' << image_height << std::endl;
        std::cout << 255 << std::endl;
        for (int j = image_height - 1; j >= 0; --j) {
            for (int i = 0; i < image_width; ++i) {
                int buffer_index = j * image_width + i;
                write_color(std::cout, render_data.buffer[buffer_index], samples_per_pixel);
            }
            float percentage_completed = (((float)image_height - (float)j) / (float)image_height)*100.0;
            std::cerr << "[" << (int)percentage_completed << "%] outputting completed" << std::endl;
        }
    } else if (output_type == 1) {
        struct RGB data[image_height][image_width];
        for (int j = image_height - 1 ; j >= 0 ; j-- ) {
            for (int i = 0; i < image_width; i++) {
                int buffer_index = j * image_width + i;
                color pixel_color = color_to_256(render_data.buffer[buffer_index], samples_per_pixel);

                data[image_height - j - 1][i].R = pixel_color.x();
                data[image_height - j - 1][i].G = pixel_color.y();
                data[image_height - j - 1][i].B = pixel_color.z();
            }
        }
        stbi_write_jpg("image.jpg", image_width, image_height, 3, data, 100);
    } else if (output_type == 2) {
        write_png("image.png", image_width, image_height, samples_per_pixel, render_data.buffer);
    }

    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
    double time_seconds = elapsed_time / 1000.0;

    std::cerr << "\nCompleted render of scene. Render time: " << time_seconds << " seconds" << "\n";
}

/**
 * @brief modified version of other output that takes in CLI arguments and modifies behaviour.
 * @note eventually should REPLACE the other one. The other one exists to keep other scenes intact.
 * @note In the future, RenderData can be replaced by Config (or the other way around).
 * The render functions should be modified to take in a std::vector<color> buffer(image_width * image_height);
 * since that is the only property of RenderData needed that DOES NOT EXIST in Config.
*/
void output(RenderData& render_data, Camera& cam, std::shared_ptr<Scene> scene_ptr, Config& config) {
    int image_height = render_data.image_height;
    int image_width = render_data.image_width;
    int samples_per_pixel = render_data.samples_per_pixel;

    auto start_time = std::chrono::high_resolution_clock::now();
    render_data.completed_lines = 0;

    std::function<void(int, int, std::shared_ptr<Scene>, RenderData&, Camera)> render_function;

    if (config.vectorization == 0) { render_function = render_scanlines; }
    else if (config.vectorization == 4) { render_function = render_scanlines_sse; }
    else if (config.vectorization == 8) { render_function = render_scanlines_avx; }
    else if (config.vectorization == 16) { render_function = render_scanlines_avx; } // replace with 16 batch render_scanlines when made
    else { render_function = render_scanlines; }
    if (!config.multithreading) {
        render_function(image_height, image_height-1, scene_ptr, render_data, cam);
    } else {
        int num_threads;
        if (config.threads == -1) {
            // Threading approach? : Divide the scanlines into N blocks
            num_threads = std::thread::hardware_concurrency() - 1;
        } else {
            num_threads = config.threads;
        }
        // Image height is the number of scanlines, suppose image_height = 800
        const int lines_per_thread = image_height / num_threads;
        const int leftOver = image_height % num_threads;
        // The first <num_threads> threads are dedicated <lines_per_thread> lines, and the last thread is dedicated to <leftOver>

        std::vector<color> pixel_colors;
        std::vector<std::thread> threads;

        for (int i=0; i < num_threads; i++) {
            // In the first thead, we want the first lines_per_thread lines to be rendered
            threads.emplace_back(render_function,lines_per_thread,(image_height-1) - (i * lines_per_thread), scene_ptr, std::ref(render_data),cam);
        }
        threads.emplace_back(render_function,leftOver,(image_height-1) - (num_threads * lines_per_thread), scene_ptr, std::ref(render_data),cam);

        for (auto &thread : threads) {
            thread.join();
        }

        if (config.verbose) {std::cerr << "Joining all threads" << std::endl;}
        threads.clear();
    }
    
    // PPM outputting. No current support for JPG and PNG.
    if (config.outputType == "ppm") {
        std::ofstream outFile(config.outputPath);
        if (!outFile.is_open()) {throw std::runtime_error("Could not open file: " + config.outputPath);}
        outFile << "P3" << std::endl;
        outFile << image_width << ' ' << image_height << std::endl;
        outFile << 255 << std::endl;
        for (int j = image_height - 1; j >= 0; --j) {
            for (int i = 0; i < image_width; ++i) {
                int buffer_index = j * image_width + i;
                write_color(outFile, render_data.buffer[buffer_index], samples_per_pixel);
            }
            float percentage_completed = (((float)image_height - (float)j) / (float)image_height)*100.0;
            if (config.verbose) {
                std::cerr << "[" << (int)percentage_completed << "%] outputting completed" << std::endl;
            }
        }
        outFile.close();
    } else if (config.outputType == "jpg") {
        struct RGB data[image_height][image_width];
        for (int j = image_height - 1 ; j >= 0 ; j-- ) {
            for (int i = 0; i < image_width; i++) {
                int buffer_index = j * image_width + i;
                color pixel_color = color_to_256(render_data.buffer[buffer_index], samples_per_pixel);

                data[image_height - j - 1][i].R = pixel_color.x();
                data[image_height - j - 1][i].G = pixel_color.y();
                data[image_height - j - 1][i].B = pixel_color.z();
            }
        }
        if (config.outputPath == "image.ppm") {
            stbi_write_jpg("image.jpg", image_width, image_height, 3, data, 100);
        } else {
            stbi_write_jpg(config.outputPath.c_str(), image_width, image_height, 3, data, 100);
        }
    } else if (config.outputType == "png") {
        if (config.outputPath == "image.ppm") {
            write_png("image.png", image_width, image_height, samples_per_pixel, render_data.buffer);
        } else {
            write_png(config.outputPath.c_str(), image_width, image_height, samples_per_pixel, render_data.buffer);
        }
    }

    if (config.verbose) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        double time_seconds = elapsed_time / 1000.0;

        std::ofstream debugFile(config.debugFile, std::ios::app);
        if (!debugFile.is_open()) {throw std::runtime_error("Could not open file: " + config.debugFile);}
        outputRenderInfo(debugFile, config, render_data, time_seconds);

        std::cerr << "\nCompleted render of scene. Render time: " << time_seconds << " seconds" << "\n";
    }
}

void random_spheres() {
    RenderData render_data; 

    const auto aspect_ratio = 3.0 / 2.0;
    setRenderData(render_data, aspect_ratio, 1200, 50, 50);

    // Set up Camera
    point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;

    Camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    setup_benchmark_scene(scene_ptr, device);

    // When scene construction is finished, the device is no longer needed.
    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}

void two_spheres() {
    // Set RenderData
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 400, 50, 50);

    // Set up Camera
    point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0001;
    
    Camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Set World
    auto checker = make_shared<checker_texture>(0.8, color(.2, .3, .1), color(.9, .9, .9));
    auto checkered_surface = make_shared<lambertian>(checker);
    auto sphere1 = make_shared<SpherePrimitive>(point3(0,-10, 0), checkered_surface, 10, device);
    auto sphere2 = make_shared<SpherePrimitive>(point3(0,10, 0), checkered_surface, 10, device);
    scene_ptr->add_primitive(sphere1);
    scene_ptr->add_primitive(sphere2);

    scene_ptr->commitScene();

    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}

void earth() {
    // Set RenderData
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 400, 50, 50);

    // Set up Camera
    point3 lookfrom(0,0,12);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0001;
    
    Camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Set World
    auto earth_texture = make_shared<image_texture>("../images/earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<SpherePrimitive>(point3(0,0,0), earth_surface, 2, device);
    unsigned int groundID = scene_ptr->add_primitive(globe);

    scene_ptr->commitScene();

    // When scene construction is finished, the device is no longer needed.
    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}

/**
 * @brief loads "scene.csr" in the same directory.
 * @note see example.csr in cypraeno/csr_schema repository
*/
void load_example(Config& config) {
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

void quads() {
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 400, 100, 50);

    // Set up Camera
    point3 lookfrom(0, 0, 9);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    double vfov = 80;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Materials
    auto left_red     = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<lambertian>(color(0.2, 0.8, 0.8));

    // Quads
    auto quad1 = make_shared<QuadPrimitive>(point3(-3,-2, 5), vec3(0, 0,-4), vec3(0, 4, 0), left_red, device);
    auto quad2 = make_shared<QuadPrimitive>(point3(-2,-2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green, device);
    auto quad3 = make_shared<QuadPrimitive>(point3( 3,-2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue, device);
    auto quad4 = make_shared<QuadPrimitive>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange, device);
    auto quad5 = make_shared<QuadPrimitive>(point3(-2,-3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal, device);

    scene_ptr->add_primitive(quad1);
    scene_ptr->add_primitive(quad2);
    scene_ptr->add_primitive(quad3);
    scene_ptr->add_primitive(quad4);
    scene_ptr->add_primitive(quad5);
    
    scene_ptr->commitScene();

    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}


void simple_light() {
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 400, 100, 50);

    // Set up Camera
    point3 lookfrom(26,3,6);
    point3 lookat(0,2,0);
    vec3 vup(0,1,0);
    double vfov = 20;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);
    
    // Materials
    auto red     = make_shared<lambertian>(color(1.0, 0.2, 0.2)); // replace with noise once implemented
    auto green   = make_shared<lambertian>(color(0.2, 1.0, 0.2)); // replace with noise once implemented

    auto sphere1 = make_shared<SpherePrimitive>(point3(0,-1000,0), red, 1000, device);
    auto sphere2 = make_shared<SpherePrimitive>(point3(0,2,0), green, 2, device);

    auto lightmaterial = make_shared<emissive>(color(6,6,6));
    auto lightsphere = make_shared<SpherePrimitive>(point3(0,7,0), lightmaterial, 2, device);
    auto lightquad = make_shared<QuadPrimitive>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), lightmaterial, device);

    // Add to Scene
    scene_ptr->add_primitive(lightquad);
    scene_ptr->add_primitive(lightsphere);
    scene_ptr->add_primitive(sphere1);
    scene_ptr->add_primitive(sphere2);

    scene_ptr->commitScene();

    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}

void cornell_box() {
    RenderData render_data; 
    const auto aspect_ratio = 1.0;
    setRenderData(render_data, aspect_ratio, 600, 20, 20);

    // Set up Camera
    point3 lookfrom(278, 278, -800);
    point3 lookat(278, 278, 0);
    vec3 vup(0,1,0);
    double vfov = 40;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Materials
    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto lightmaterial = make_shared<emissive>(color(15,15,15));

    auto quad1 = make_shared<QuadPrimitive>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green, device);
    auto quad2 = make_shared<QuadPrimitive>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red, device);
    auto quad3 = make_shared<QuadPrimitive>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), lightmaterial, device);
    auto quad4 = make_shared<QuadPrimitive>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white, device);
    auto quad5 = make_shared<QuadPrimitive>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white, device);
    auto quad6 = make_shared<QuadPrimitive>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white, device);

    // Add to Scene
    scene_ptr->add_primitive(quad1);
    scene_ptr->add_primitive(quad2);
    scene_ptr->add_primitive(quad3);
    scene_ptr->add_primitive(quad4);
    scene_ptr->add_primitive(quad5);
    scene_ptr->add_primitive(quad6);

    scene_ptr->commitScene();

    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}

void instances() {
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 600, 200, 50);

    // Set up Camera
    point3 lookfrom(10, 0, 0);
    point3 lookat(0, 0, 0);
    vec3 vup(0,1,0);
    double vfov = 60;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Create initial sphere
    auto red     = make_shared<lambertian>(color(1.0, 0.2, 0.2)); // replace with noise once implemented
    auto sphere1 = make_shared<SpherePrimitive>(point3(0,0,3), red, 1, device);
    scene_ptr->add_primitive(sphere1);

    // Create an INSTANCE of the original sphere
    float transform[12] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, -6
    }; // results in the isntance being at (0,0,-3)
    auto sphere_instance = make_shared<SpherePrimitiveInstance>(sphere1, transform, device);
    scene_ptr->add_primitive_instance(sphere_instance, device);

    scene_ptr->commitScene();
    rtcReleaseDevice(device);
    output(render_data, cam, scene_ptr);
}

void two_perlin_spheres(){
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 400, 100, 50);

    // Set up Camera
    point3 lookfrom(13, 2, 3);
    point3 lookat(0, 0, 0);
    vec3 vup(0,1,0);
    double vfov = 20;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Materials
    auto pertext = make_shared<noise_texture>(4);
    auto perlin_mtl = make_shared<lambertian>(pertext);

    auto sphere1 = make_shared<SpherePrimitive>(point3(0,-1000, 0), perlin_mtl, 1000, device);
    auto sphere2 = make_shared<SpherePrimitive>(point3(0,2, 0), perlin_mtl, 2, device);

    scene_ptr->add_primitive(sphere1);
    scene_ptr->add_primitive(sphere2);

    scene_ptr->commitScene();
    rtcReleaseDevice(device);

    output(render_data, cam, scene_ptr);
}

void instances_quads() {
    RenderData render_data; 
    const auto aspect_ratio = 16.0 / 9.0;
    setRenderData(render_data, aspect_ratio, 600, 200, 50);

    // Set up Camera
    point3 lookfrom(10, 0, 0);
    point3 lookat(0, 0, 0);
    vec3 vup(0,1,0);
    double vfov = 60;
    double aperture = 0.0001;
    double dist_to_focus = 10.0;

    Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Simple usage of creating a Scene
    RTCDevice device = initializeDevice();
    auto scene_ptr = make_shared<Scene>(device, cam);

    // Create initial sphere
    auto red   = make_shared<lambertian>(color(1.0, 0.2, 0.2)); // replace with noise once implemented
    auto quad1 = make_shared<QuadPrimitive>(point3(0,0,3), vec3(0, 3, 0), vec3(3, 0, 0), red, device);
    scene_ptr->add_primitive(quad1);

    // Create an INSTANCE of the original sphere
    float transform[12] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, -6
    }; // results in the isntance being at (0,0,-3)
    auto quad_instance = make_shared<QuadPrimitiveInstance>(quad1, transform, device);
    scene_ptr->add_primitive_instance(quad_instance, device);

    scene_ptr->commitScene();
    rtcReleaseDevice(device);
    output(render_data, cam, scene_ptr);
}
// void instances_test(){
// // Set RenderData
//     RenderData render_data; 
//     const auto aspect_ratio = 3.0 / 2.0;
//     setRenderData(render_data, aspect_ratio, 1200, 100, 50);
//     RTCDevice device = initializeDevice();


//     // Set up Camera
//     point3 lookfrom(13, 2, 3);
//     point3 lookat(0, 0, 0);
//     vec3 vup(0,1,0);
//     double vfov = 20;
//     double aperture = 0.0001;
//     double dist_to_focus = 10.0;

//     Camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);
//     auto scene_ptr = make_shared<Scene>(device, cam);

//     // Ground
//     auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5)); // Solid gray Lambertian material
//     auto ground = (make_shared<SpherePrimitive>(point3(0, -1000, 0), ground_material, 1000, device));
//     scene_ptr->add_primitive(ground);

//     // First Sphere with instance transformations
//     point3 center1(0, 0.2, 0);
//     auto material1 = make_shared<lambertian>(color(0.8, 0.4, 0.4)); // Reddish Lambertian material
//     auto sphere1 = make_shared<SpherePrimitive>(center1, material1, 0.2, device);
//     scene_ptr->add_primitive(sphere1);

//        // Create an INSTANCE of the original sphere
//     float transform[12] = {
//         1, 0, 0, 0,
//         0, 1, 0, 0,
//         0, 0, 1, -6
//     }; // results in the isntance being at (0,0,-3)
//     auto sphere_instance = make_shared<SpherePrimitiveInstance>(sphere1, transform, device);
//     scene_ptr->add_primitive_instance(sphere_instance, device);

//     // // Second Sphere with instance transformations
//     // point3 center2(0, 0.6, 0); // Placed above the first sphere
//     // auto material2 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0); // Metal material
//     // auto quad1 = make_shared<QuadPrimitive>(center2, 0.4, material2, create_still_timeline(center2));
//     // sphere2 = make_shared<translate>(sphere2, vec3(-1, 0, 0)); // Translate by (-1, 0, 0)
//     // sphere2 = make_shared<rotate_y>(sphere2, -45); // Rotate around y-axis by -45 degrees
//     // // world.add(sphere2);
//     scene_ptr->commitScene();
//     rtcReleaseDevice(device);
    

//     output(render_data, cam, scene_ptr);
    
// }
int main(int argc, char* argv[]) {
    Config config = parseArguments(argc, argv);
    switch (521) {
        case 30:  random_spheres(); break;
        case 48:  two_spheres();    break;
        case 481:  earth();          break;
        case 50:  quads();          break;
        case 80:  load_example(config);   break;
        case 51:  simple_light();   break;
        case 511:  cornell_box();    break;
        case 49: two_perlin_spheres(); break;
        case 52: instances(); break;
        case 521: instances_quads(); break;
    }
}

