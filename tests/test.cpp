#include "lpr.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char** argv) {
    LprEngine engine;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image_path>\n";
        return 1;
    }

    const char* image_path = argv[1];

    int width, height, channels;

    unsigned char* data = stbi_load(image_path, &width, &height, &channels, 3);
    if (!data) {
        std::cerr << "Erro ao carregar imagem: " << image_path << "\n";
        return 1;
    }

    // RGB -> BGR
    for (int i = 0; i < width * height; i++) {
        std::swap(data[i * 3 + 0], data[i * 3 + 2]);
    }


    auto results = engine.process(data, width, height);

    if (results.empty()) {
        std::cout << "Nenhuma placa detectada\n";
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