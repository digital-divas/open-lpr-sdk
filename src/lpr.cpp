#include "lpr.h"
#include <iostream>
#include <onnxruntime_cxx_api.h>

LprEngine::LprEngine() {
    std::cout << "LPR Engine initialized" << std::endl;
}

std::vector<LprResult> LprEngine::process(
    const unsigned char* frame,
    int width,
    int height
) {

    std::vector<LprResult> results;

    LprResult r;

    r.plate = "FXL7E66";
    r.confidence = 0.99;

    r.x1 = 100;
    r.y1 = 200;
    r.x2 = 300;
    r.y2 = 250;

    results.push_back(r);

    return results;
}