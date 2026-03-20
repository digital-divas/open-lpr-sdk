#include "lpr.h"
#include "models.h"

#include <onnxruntime_cxx_api.h>

#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <thread>
#include <chrono>

#ifdef __ANDROID__

#include <android/log.h>

#define LOG_TAG "LPR_SDK"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define LOGD_VERBOSE(verbose, ...) LOGD(__VA_ARGS__)

#else

#include <iostream>

#define LOGD_VERBOSE(verbose, ...) \
    do { \
        if (verbose) { \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while(0)

#define LOGD(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) printf(__VA_ARGS__); printf("\n")

#endif

// =========================
// Utils básicos
// =========================
static inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

struct Image {
    int w, h, c;
    std::vector<uint8_t> data;
};

// Bilinear resize
static Image resize_bilinear(const Image& src, int new_w, int new_h) {
    Image dst{new_w, new_h, src.c};
    dst.data.resize(new_w * new_h * src.c);

    float x_ratio = float(src.w) / new_w;
    float y_ratio = float(src.h) / new_h;

    for (int j = 0; j < new_h; j++) {
        for (int i = 0; i < new_w; i++) {
            float gx = (i + 0.5f) * x_ratio - 0.5f;
            float gy = (j + 0.5f) * y_ratio - 0.5f;

            int x0 = std::floor(gx);
            int y0 = std::floor(gy);
            int x1 = std::min(x0 + 1, src.w - 1);
            int y1 = std::min(y0 + 1, src.h - 1);

            float dx = gx - x0;
            float dy = gy - y0;

            x0 = std::max(x0, 0);
            y0 = std::max(y0, 0);

            for (int c = 0; c < src.c; c++) {
                float v00 = src.data[(y0 * src.w + x0) * src.c + c];
                float v01 = src.data[(y0 * src.w + x1) * src.c + c];
                float v10 = src.data[(y1 * src.w + x0) * src.c + c];
                float v11 = src.data[(y1 * src.w + x1) * src.c + c];

                float val =
                    v00 * (1 - dx) * (1 - dy) +
                    v01 * dx * (1 - dy) +
                    v10 * (1 - dx) * dy +
                    v11 * dx * dy;

                dst.data[(j * new_w + i) * src.c + c] = (uint8_t)val;
            }
        }
    }

    return dst;
}

// BGR → RGB (igual OpenCV pipeline)
static void bgr_to_rgb(Image& img) {
    for (int i = 0; i < img.w * img.h; i++) {
        std::swap(img.data[i * 3 + 0], img.data[i * 3 + 2]);
    }
}

static Image crop_image(const Image& src, int x1, int y1, int x2, int y2) {
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(src.w, x2);
    y2 = std::min(src.h, y2);

    int w = x2 - x1;
    int h = y2 - y1;

    Image out{w, h, 3};
    out.data.resize(w * h * 3);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            for (int c = 0; c < 3; c++) {
                out.data[(y * w + x) * 3 + c] =
                    src.data[((y + y1) * src.w + (x + x1)) * 3 + c];
            }
        }
    }

    return out;
}

static Image ocr_preprocess(const Image& img, int target_w = 128, int target_h = 64) {

    float scale = std::min(float(target_w) / img.w, float(target_h) / img.h);

    int new_w = int(img.w * scale);
    int new_h = int(img.h * scale);

    Image resized = resize_bilinear(img, new_w, new_h);

    Image canvas{target_w, target_h, 3};
    canvas.data.resize(target_w * target_h * 3, 0);

    int x_off = (target_w - new_w) / 2;
    int y_off = (target_h - new_h) / 2;

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            for (int c = 0; c < 3; c++) {
                canvas.data[((y + y_off) * target_w + (x + x_off)) * 3 + c] =
                    resized.data[(y * new_w + x) * 3 + c];
            }
        }
    }

    // BGR → RGB (igual antes)
    bgr_to_rgb(canvas);

    return canvas;
}

static std::pair<std::string, float> ctc_decode(const float* data, int T, int num_classes) {

    std::string plate;
    std::vector<float> confs;

    for (int t = 0; t < T; t++) {
        const float* row = data + t * num_classes;

        int best = std::max_element(row, row + num_classes) - row;

        if (best == num_classes - 1) continue; // blank

        char c = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_"[best];
        plate += c;
        confs.push_back(row[best]);
    }

    float conf = confs.empty() ? 0.f :
        std::accumulate(confs.begin(), confs.end(), 0.f) / confs.size();

    return {plate, conf};
}

// Letterbox (YOLO style)
struct LetterboxResult {
    Image img;
    float ratio;
    float dw;
    float dh;
};

static LetterboxResult letterbox(const Image& src, int new_w = 640, int new_h = 640) {
    float r = std::min(float(new_w) / src.w, float(new_h) / src.h);

    int rw = int(std::round(src.w * r));
    int rh = int(std::round(src.h * r));

    Image resized = resize_bilinear(src, rw, rh);

    Image out{new_w, new_h, 3};
    out.data.resize(new_w * new_h * 3, 114);

    int dw = (new_w - rw) / 2;
    int dh = (new_h - rh) / 2;

    for (int y = 0; y < rh; y++) {
        for (int x = 0; x < rw; x++) {
            for (int c = 0; c < 3; c++) {
                out.data[((y + dh) * new_w + (x + dw)) * 3 + c] =
                    resized.data[(y * rw + x) * 3 + c];
            }
        }
    }

    return {out, r, float(dw), float(dh)};
}

// HWC → CHW + normalize
static std::vector<float> to_tensor(const Image& img) {
    int H = img.h, W = img.w;

    std::vector<float> out(3 * H * W);

    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                out[c * H * W + y * W + x] =
                    img.data[(y * W + x) * 3 + c] / 255.0f;
            }
        }
    }

    return out;
}

// =========================
// NMS
// =========================

struct Detection {
    float x1, y1, x2, y2;
    float conf;
};

static float iou(const Detection& a, const Detection& b) {
    float x1 = std::max(a.x1, b.x1);
    float y1 = std::max(a.y1, b.y1);
    float x2 = std::min(a.x2, b.x2);
    float y2 = std::min(a.y2, b.y2);

    float inter = std::max(0.f, x2 - x1) * std::max(0.f, y2 - y1);
    float areaA = (a.x2 - a.x1) * (a.y2 - a.y1);
    float areaB = (b.x2 - b.x1) * (b.y2 - b.y1);

    return inter / (areaA + areaB - inter + 1e-6f);
}

static std::vector<Detection> nms(std::vector<Detection>& dets, float thresh) {
    std::sort(dets.begin(), dets.end(), [](auto& a, auto& b) {
        return a.conf > b.conf;
    });

    std::vector<Detection> out;

    for (auto& d : dets) {
        bool keep = true;
        for (auto& o : out) {
            if (iou(d, o) > thresh) {
                keep = false;
                break;
            }
        }
        if (keep) out.push_back(d);
    }

    return out;
}

LprEngine::LprEngine(bool verbose): verboseLogs(verbose)  {
    LOGD_VERBOSE(verboseLogs, "LPR Engine initializing...");

    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    sessionOptions.SetIntraOpNumThreads(0);

    sessionOptions.AddConfigEntry("session.intra_op.allow_spinning", "0");

    try {
        auto num_threads = std::max(1u, std::thread::hardware_concurrency());

        LOGD_VERBOSE(verboseLogs, "Threads: %d", num_threads);

        sessionOptions.AppendExecutionProvider(
            "XNNPACK",
            {{"intra_op_num_threads", std::to_string(num_threads)}}
        );

        LOGD_VERBOSE(verboseLogs, "XNNPACK enabled");

    } catch (...) {
        LOGD_VERBOSE(verboseLogs, "XNNPACK not available, fallback to CPU");
    }

#ifdef __ANDROID__

    // uint32_t nnapi_flags = 0;

    // OrtStatus* status = OrtSessionOptionsAppendExecutionProvider_Nnapi(
    //     sessionOptions,
    //     nnapi_flags
    // );

    // if (status != nullptr) {
    //     LOGD("[LPR] NNAPI not available, fallback to CPU");
    //     Ort::GetApi().ReleaseStatus(status);
    // } else {
    //     LOGD("[LPR] NNAPI enabled");
    // }

#endif

    // ===== DETECTOR =====
    LOGD_VERBOSE(verboseLogs, "[LPR] Loading detector model...");
    detector = std::make_unique<Ort::Session>(
        env,
        (const void*)whoami_onnx,
        (size_t)whoami_onnx_len,
        sessionOptions
    );

    // ===== OCR =====
    LOGD_VERBOSE(verboseLogs, "[LPR] Loading OCR model...");
    ocr = std::make_unique<Ort::Session>(
        env,
        (const void*)cct_xs_v1_global_onnx,
        (size_t)cct_xs_v1_global_onnx_len,
        sessionOptions
    );

    Ort::AllocatorWithDefaultOptions allocator;

    auto d_in = detector->GetInputNameAllocated(0, allocator);
    auto d_out = detector->GetOutputNameAllocated(0, allocator);

    detectorInputName = std::string(d_in.get());
    detectorOutputName = std::string(d_out.get());

    auto o_in = ocr->GetInputNameAllocated(0, allocator);
    auto o_out = ocr->GetOutputNameAllocated(0, allocator);

    ocrInputName = std::string(o_in.get());
    ocrOutputName = std::string(o_out.get());

    LOGD_VERBOSE(verboseLogs, "LPR Engine ready");
}

// =========================
// PROCESS (DETECTOR REAL)
// =========================

std::vector<LprResult> LprEngine::process(
    const unsigned char* frame,
    int width,
    int height
) {
    auto t0 = now_ms();

    Image img{width, height, 3};
    img.data.assign(frame, frame + width * height * 3);
    auto t1 = now_ms();

    auto lb = letterbox(img);
    auto t2 = now_ms();

    bgr_to_rgb(lb.img);
    auto t3 = now_ms();

    auto blob = to_tensor(lb.img);
    auto t4 = now_ms();

    std::vector<int64_t> shape = {1, 3, lb.img.h, lb.img.w};

    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    auto input = Ort::Value::CreateTensor<float>(
        mem,
        blob.data(),
        blob.size(),
        shape.data(),
        shape.size()
    );
    auto t5 = now_ms();

    const char* in_names[] = {detectorInputName.c_str()};
    const char* out_names[] = {detectorOutputName.c_str()};

    auto outputs = detector->Run(
        Ort::RunOptions{nullptr},
        in_names,
        &input,
        1,
        out_names,
        1
    );
    auto t6 = now_ms();

    LOGD_VERBOSE(
        verboseLogs,
        "TIMING:\n"
        "copy:        %lld ms\n"
        "letterbox:   %lld ms\n"
        "bgr2rgb:     %lld ms\n"
        "to_tensor:   %lld ms\n"
        "tensor:      %lld ms\n"
        "onnx:        %lld ms\n"
        "total:       %lld ms",
        t1 - t0,
        t2 - t1,
        t3 - t2,
        t4 - t3,
        t5 - t4,
        t6 - t5,
        t6 - t0
    );

    auto shape_out = outputs[0].GetTensorTypeAndShapeInfo().GetShape();

    int num_attrs = (int)shape_out[1];
    int num_preds = (int)shape_out[2];

    const float* raw = outputs[0].GetTensorData<float>();

    std::vector<Detection> dets;

    for (int i = 0; i < num_preds; i++) {
        float best = 0.f;
        for (int c = 4; c < num_attrs; c++) {
            best = std::max(best, raw[c * num_preds + i]);
        }

        if (best < 0.25f) continue;

        float cx = raw[0 * num_preds + i];
        float cy = raw[1 * num_preds + i];
        float bw = raw[2 * num_preds + i];
        float bh = raw[3 * num_preds + i];

        float x1 = ((cx - bw / 2) - lb.dw) / lb.ratio;
        float y1 = ((cy - bh / 2) - lb.dh) / lb.ratio;
        float x2 = ((cx + bw / 2) - lb.dw) / lb.ratio;
        float y2 = ((cy + bh / 2) - lb.dh) / lb.ratio;

        dets.push_back({x1, y1, x2, y2, best});
    }

    auto final_dets = nms(dets, 0.45f);

    std::vector<LprResult> results;

    for (auto& d : final_dets) {
        if (d.conf >= 0.75) {
            LprResult r;

            Image crop = crop_image(img, d.x1, d.y1, d.x2, d.y2);

            Image ocr_img = ocr_preprocess(crop);

            // NHWC uint8
            std::vector<int64_t> ocr_shape = {1, 64, 128, 3};

            Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            auto ocr_tensor = Ort::Value::CreateTensor<uint8_t>(
                mem,
                ocr_img.data.data(),
                ocr_img.data.size(),
                ocr_shape.data(),
                ocr_shape.size()
            );

            const char* ocr_in[] = {ocrInputName.c_str()};
            const char* ocr_out[] = {ocrOutputName.c_str()};

            auto ocr_outputs = ocr->Run(
                Ort::RunOptions{nullptr},
                ocr_in,
                &ocr_tensor,
                1,
                ocr_out,
                1
            );

            auto shape = ocr_outputs[0].GetTensorTypeAndShapeInfo().GetShape();

            int T = (int)shape[1];
            int num_classes = (int)shape[2];

            auto [plate, conf] = ctc_decode(
                ocr_outputs[0].GetTensorData<float>(),
                T,
                num_classes
            );

            r.plate = plate;
            r.confidence = conf;

            r.x1 = std::max(0, (int)d.x1);
            r.y1 = std::max(0, (int)d.y1);
            r.x2 = std::min(width, (int)d.x2);
            r.y2 = std::min(height, (int)d.y2);

            results.push_back(r);
        }
    }

    return results;
}