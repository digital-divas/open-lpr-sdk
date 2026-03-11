#include "lpr.h"
#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    LprEngine engine;

    cv::Mat image = cv::imread("../dataset/FXL7E66.jpg");
    if (image.empty()) {
        std::cerr << "Erro ao carregar imagem\n";
        return 1;
    }

    auto results = engine.process(image.data, image.cols, image.rows);

    if (results.empty()) {
        std::cout << "Nenhuma placa detectada\n";
        return 0;
    }

    for (auto& r : results) {
        std::cout << "Plate:      " << r.plate      << std::endl;
        std::cout << "Confidence: " << r.confidence << std::endl;
        std::cout << "BBox:       " << r.x1 << " " << r.y1
                  << " "            << r.x2 << " " << r.y2 << std::endl;
    }
}
