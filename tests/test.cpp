#include "lpr.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char** argv) {
    bool verbose = false;
    const char* image_path = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-V") == 0) {
            verbose = true;
        } else {
            image_path = argv[i];
        }
    }

    if (!image_path) {
        std::cerr << "Usage: " << argv[0] << " [-V] <image_path>\n";
        return 1;
    }

    LprEngine engine(verbose);

    int width, height, channels;

    unsigned char* data = stbi_load(image_path, &width, &height, &channels, 3);
    if (!data) {
        std::cerr << "Error loading image: " << image_path << "\n";
        return 1;
    }

    // RGB -> BGR
    for (int i = 0; i < width * height; i++) {
        std::swap(data[i * 3 + 0], data[i * 3 + 2]);
    }


    auto results = engine.process(data, width, height);

    if (results.empty()) {
        std::cout << "No plate detected\n";
    } else {
        for (auto& r : results) {
            std::cout << "Plate:      " << r.plate      << std::endl;
            std::cout << "Confidence: " << r.confidence << std::endl;
            std::cout << "BBox:       "
                      << r.x1 << " " << r.y1 << " "
                      << r.x2 << " " << r.y2 << std::endl;
        }
    }

    stbi_image_free(data);

    return 0;
}