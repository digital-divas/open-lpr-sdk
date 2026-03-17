#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define LPR_API __declspec(dllexport)
#else
#define LPR_API __attribute__((visibility("default")))
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

LPR_API LprEngineHandle* lpr_create();

LPR_API void lpr_destroy(LprEngineHandle* engine);

LPR_API int lpr_process(
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