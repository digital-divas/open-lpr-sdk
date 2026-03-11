#pragma once

#include <string>
#include <vector>

struct LprResult {

    std::string plate;
    float confidence;

    int x1;
    int y1;
    int x2;
    int y2;

};

class LprEngine {

public:

    LprEngine();

    std::vector<LprResult> process(
        const unsigned char* frame,
        int width,
        int height
    );

};