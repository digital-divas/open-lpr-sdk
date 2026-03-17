#include "lpr.h"
#include "models.h"
#include <iostream>
#include <onnxruntime_cxx_api.h>

LprEngine::LprEngine() {
    std::cout << "LPR Engine initializing..." << std::endl;

    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    sessionOptions.SetIntraOpNumThreads(1);

    // ===== DETECTOR =====
    std::cout << "[LPR] Loading detector model..." << std::endl;
    detector = std::make_unique<Ort::Session>(
        env,
        (const void*)whoami_onnx,
        (size_t)whoami_onnx_len,
        sessionOptions
    );

    // ===== OCR =====
    std::cout << "[LPR] Loading OCR model..." << std::endl;
    ocr = std::make_unique<Ort::Session>(
        env,
        (const void*)cct_xs_v1_global_onnx,
        (size_t)cct_xs_v1_global_onnx_len,
        sessionOptions
    );

    Ort::AllocatorWithDefaultOptions allocator;

    // detector
    auto d_in = detector->GetInputNameAllocated(0, allocator);
    auto d_out = detector->GetOutputNameAllocated(0, allocator);

    detectorInputName = d_in.get();
    detectorOutputName = d_out.get();

    // ocr
    auto o_in = ocr->GetInputNameAllocated(0, allocator);
    auto o_out = ocr->GetOutputNameAllocated(0, allocator);

    ocrInputName = o_in.get();
    ocrOutputName = o_out.get();

    std::cout << "LPR Engine ready" << std::endl;
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