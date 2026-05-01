#pragma once

#include <string>
#include <vector>
#include <memory>

struct LprResult {

    std::string plate;
    float confidence;

    int x1;
    int y1;
    int x2;
    int y2;

};

struct Detection {
    float x1, y1, x2, y2;
    float conf;
    int class_id;
};

class LprEngine {

public:

    explicit LprEngine(bool verboseLogs = false);
    ~LprEngine();

    LprEngine(const LprEngine&) = delete;

    LprEngine& operator=(const LprEngine&) = delete;

    LprEngine(LprEngine&&) noexcept = default;

    LprEngine& operator=(LprEngine&&) noexcept = default;

    std::vector<LprResult> process(
        const unsigned char* frame,
        int width,
        int height
    );

    std::vector<Detection> run_yolo(
        const unsigned char* frame,
        int width,
        int height
    );

    std::vector<LprResult> run_plate_detector(
        const unsigned char* frame,
        int width,
        int height
    );

private:
    struct LprEngineImpl;
    std::unique_ptr<LprEngineImpl> impl;

    bool verboseLogs = false;
};