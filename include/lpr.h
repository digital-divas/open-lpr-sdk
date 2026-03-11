#pragma once

#include <string>
#include <vector>
#include <memory>

struct LprResult {
    std::string plate;
    float confidence;
    int x1, y1, x2, y2;
};

class LprEngine {
public:
    LprEngine();
    ~LprEngine();

    std::vector<LprResult> process(const unsigned char* frame,
                                   int width, int height,
                                   float confidence = 0.75f);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};