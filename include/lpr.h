#pragma once

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>

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

    explicit LprEngine(bool verboseLogs = false);

    std::vector<LprResult> process(
        const unsigned char* frame,
        int width,
        int height
    );

private:
    Ort::Env env;
    Ort::SessionOptions sessionOptions;

    std::unique_ptr<Ort::Session> detector;
    std::unique_ptr<Ort::Session> ocr;

    std::string detectorInputName;
    std::string detectorOutputName;

    std::string ocrInputName;
    std::string ocrOutputName;

    bool verboseLogs = false;
};