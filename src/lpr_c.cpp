#include "lpr_c.h"
#include "lpr.h"

#include <cstring>

struct LprEngineHandle {
    LprEngine engine;
};

LprEngineHandle* lpr_create() {

    auto handle = new LprEngineHandle();

    return handle;
}

void lpr_destroy(LprEngineHandle* engine) {

    delete engine;
}

int lpr_process(
    LprEngineHandle* engine,
    const uint8_t* frame,
    int width,
    int height,
    LprDetection* out_results,
    int max_results
) {

    auto results = engine->engine.process(frame, width, height);

    int count = 0;

    for (auto& r : results) {

        if (count >= max_results)
            break;

        std::strncpy(out_results[count].plate, r.plate.c_str(), 15);
        out_results[count].plate[15] = '\0';

        out_results[count].confidence = r.confidence;

        out_results[count].x1 = r.x1;
        out_results[count].y1 = r.y1;
        out_results[count].x2 = r.x2;
        out_results[count].y2 = r.y2;

        count++;
    }

    return count;
}