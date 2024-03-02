#include <iostream>
#include <string>
#include <sstream>

/**
 * @brief structure to hold all relevant data determined by flags.
 * Automatically holds defaults.
 * @note A little redundant with the existence of RenderData. Unsure how to reconcile.
*/
struct Config {

    // Standard flags
    int samples_per_pixel = 50;
    int max_depth = 50;
    std::string inputFile = "example.csr";
    std::string outputPath = "./";
    int image_width = 1200;
    int image_height = 800;
    std::string outputType = "ppm"; // [jpg|png|ppm]
    
    // Output flags
    bool showVersion = false;
    bool showHelp = false;
    bool verbose = false;

    // Optimization flags
    bool multithreading = false;
    int threads = 1; // unused if multithreading is false. If it is set by -t, then bool multithreading is irrelevant.
    int vectorization = 0; // [NONE|4|8|16], NONE = 0    

};

/**
 * @brief given argc, argv, process and return a Config struct containing all the settings.
 * Doesn't account for some invalid input, such as:
 * -> negative samples and depth
 * -> invalid input and output paths
 * -> negative image widths and heights
 * -> outputType that doesn't match an option
 * -> unneeded or too many or too little arguments for a flag
 * -> non-integer provided for integer fields
 * -> no help messages
 * -> no checks for if threads or vectorization is supported by hardware
*/
Config parseArguments(int argc, char* argv[]) {
    Config config;
    for(int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if(arg == "-s" || arg == "--samples") {
            if(i + 1 < argc) { // Make sure we aren't at the end of argv
                config.samples_per_pixel = std::stoi(argv[++i]); // Increment 'i' and get the next argument
            }
        } else if(arg == "-d" || arg == "--depth") {
            if(i + 1 < argc) {
                config.max_depth = std::stoi(argv[++i]);
            }
        } else if(arg == "-i" || arg == "--input") {
            if(i + 1 < argc) {
                config.inputFile = argv[++i];
            }
        } else if(arg == "-o" || arg == "--output") {
            if(i + 1 < argc) {
                config.outputPath = argv[++i];
            }
        } else if(arg == "-r" || arg == "--resolution") {
            if(i + 2 < argc) { // Expect two arguments for resolution
                config.image_width = std::stoi(argv[++i]);
                config.image_height = std::stoi(argv[++i]);
            }
        } else if(arg == "-t" || arg == "--type") {
            if(i + 1 < argc) {
                config.outputType = argv[++i];
            }
        } else if(arg == "-m" || arg == "--multithreading") {
            config.multithreading = true;
        } else if(arg == "-t" || arg == "--threads") {
            if(i + 1 < argc) {
                config.threads = std::stoi(argv[++i]);
            }
        } else if(arg == "-Vx" || arg == "--vectorization") {
            if(i + 1 < argc) {
                int choice = std::stoi(argv[++i]);
                if (choice == 0 || choice == 4 || choice == 8 || choice == 16) {
                    config.vectorization = choice;
                } else {
                    throw std::runtime_error("Error: Invalid option for --vectorization [1|4|8|16]. Use '--help' for more information.");
                }
            }
        } else if(arg == "-v" || arg == "--version") {
            config.showVersion = true;
        } else if(arg == "-h" || arg == "--help") {
            config.showHelp = true;
        } else if(arg == "-V" || arg == "--verbose") {
            config.showHelp = true;
        }
    }
    return config;
}

void outputHelpGuide(std::ostream& out) {}