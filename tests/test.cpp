#include "lpr.h"
#include <iostream>

int main() {

    LprEngine engine;

    unsigned char fake_frame[100];

    auto results = engine.process(fake_frame, 1920, 1080);

    for (auto& r : results) {

        std::cout << "Plate: " << r.plate << std::endl;
        std::cout << "Confidence: " << r.confidence << std::endl;

        std::cout << "BBox: "
                  << r.x1 << " "
                  << r.y1 << " "
                  << r.x2 << " "
                  << r.y2 << std::endl;

        std::cout << std::endl;
    }

}