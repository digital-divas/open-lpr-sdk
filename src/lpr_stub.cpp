#include "lpr.h"
#include <thread>
#include <chrono>

struct LprEngine::LprEngineImpl {
};

LprEngine::LprEngine(bool verboseLogs): verboseLogs(verboseLogs), impl(std::make_unique<LprEngineImpl>()) {

}

LprEngine::~LprEngine() = default;

std::vector<LprResult> LprEngine::process(
    const unsigned char* frame,
    int width,
    int height
) {
    // simulates a processing time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return {
        {
            "ABC1234",
            0.87f,
            100, 100, 200, 150
        }
    };
}

std::vector<Detection> LprEngine::run_yolo(
    const unsigned char* frame,
    int width,
    int height
) {
    return {};
}

std::vector<LprResult> LprEngine::run_plate_detector(
    const unsigned char* frame,
    int width,
    int height
) {
    return process(frame, width, height);
}