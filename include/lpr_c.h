#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    char plate[16];
    float confidence;

    int x1;
    int y1;
    int x2;
    int y2;

} LprDetection;

typedef struct LprEngineHandle LprEngineHandle;

LprEngineHandle* lpr_create();

void lpr_destroy(LprEngineHandle* engine);

int lpr_process(
    LprEngineHandle* engine,
    const uint8_t* frame,
    int width,
    int height,
    LprDetection* out_results,
    int max_results
);

#ifdef __cplusplus
}
#endif