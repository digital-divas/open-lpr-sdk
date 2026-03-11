#include "lpr.h"

#include <numeric>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

extern "C" {
    extern const unsigned char whoami_onnx[];
    extern const unsigned int  whoami_onnx_len;
    extern const unsigned char cct_xs_v1_global_onnx[];
    extern const unsigned int  cct_xs_v1_global_onnx_len;
}

static const std::string CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

static std::vector<uint8_t> ocr_preprocess(const cv::Mat& img,
                                            int target_w = 128,
                                            int target_h = 64)
{
    int h = img.rows, w = img.cols;
    float scale = std::min(static_cast<float>(target_w) / w,
                           static_cast<float>(target_h) / h);
    int new_w = static_cast<int>(w * scale);
    int new_h = static_cast<int>(h * scale);

    cv::Mat resized;
    cv::resize(img, resized, {new_w, new_h});

    cv::Mat canvas(target_h, target_w, CV_8UC3, cv::Scalar(0, 0, 0));
    int x = (target_w - new_w) / 2;
    int y = (target_h - new_h) / 2;
    resized.copyTo(canvas(cv::Rect(x, y, new_w, new_h)));
    cv::cvtColor(canvas, canvas, cv::COLOR_BGR2RGB);

    std::vector<uint8_t> blob(target_h * target_w * 3);
    int idx = 0;
    for (int r = 0; r < target_h; ++r)
        for (int c = 0; c < target_w; ++c)
            for (int ch = 0; ch < 3; ++ch)
                blob[idx++] = canvas.at<cv::Vec3b>(r, c)[ch];
    return blob;
}

static std::pair<std::string, float> ctc_decode(const float* data, int T, int num_classes)
{
    std::string plate;
    std::vector<float> confs;
    for (int t = 0; t < T; ++t) {
        const float* row = data + t * num_classes;
        int best = static_cast<int>(std::max_element(row, row + num_classes) - row);
        if (CHARS[best] == '_') continue;
        plate += CHARS[best];
        confs.push_back(row[best]);
    }
    float conf = confs.empty()
        ? 0.f
        : std::accumulate(confs.begin(), confs.end(), 0.f) / confs.size();
    return {plate, conf};
}

struct LetterboxResult { cv::Mat img; float ratio, dw, dh; };

static LetterboxResult letterbox(const cv::Mat& src, int new_h = 640, int new_w = 640)
{
    int h = src.rows, w = src.cols;
    float r = std::min(static_cast<float>(new_h) / h,
                       static_cast<float>(new_w) / w);
    int uw = static_cast<int>(std::round(w * r));
    int uh = static_cast<int>(std::round(h * r));

    cv::Mat resized;
    cv::resize(src, resized, {uw, uh}, 0, 0, cv::INTER_LINEAR);

    float dw_f = (new_w - uw) / 2.0f;
    float dh_f = (new_h - uh) / 2.0f;

    cv::Mat padded;
    cv::copyMakeBorder(resized, padded,
        static_cast<int>(std::round(dh_f - 0.1f)),
        static_cast<int>(std::round(dh_f + 0.1f)),
        static_cast<int>(std::round(dw_f - 0.1f)),
        static_cast<int>(std::round(dw_f + 0.1f)),
        cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

    return {padded, r, dw_f, dh_f};
}

struct LprEngine::Impl {
    Ort::Env            env{ORT_LOGGING_LEVEL_WARNING, "lpr"};
    Ort::SessionOptions opts;
    Ort::Session        det_session;
    Ort::Session        ocr_session;

    static Ort::SessionOptions make_opts() {
        Ort::SessionOptions o;
        o.SetIntraOpNumThreads(4);
        o.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        return o;
    }

    Impl()
        : opts(make_opts())
        , det_session(env,
                      whoami_onnx, whoami_onnx_len,
                      opts)
        , ocr_session(env,
                      cct_xs_v1_global_onnx, cct_xs_v1_global_onnx_len,
                      opts)
    {}

    struct RawDetection { int x1, y1, x2, y2; float conf; };

    std::vector<RawDetection> run_detector(const cv::Mat& image, float min_conf)
    {
        auto lb = letterbox(image);

        cv::Mat rgb;
        cv::cvtColor(lb.img, rgb, cv::COLOR_BGR2RGB);

        int H = rgb.rows, W = rgb.cols, C = 3;
        std::vector<float> blob(C * H * W);
        for (int c = 0; c < C; ++c)
            for (int r = 0; r < H; ++r)
                for (int col = 0; col < W; ++col)
                    blob[c * H * W + r * W + col] =
                        rgb.at<cv::Vec3b>(r, col)[c] / 255.0f;

        std::vector<int64_t> shape = {1, C, H, W};
        Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::AllocatorWithDefaultOptions alloc;

        auto in_name  = det_session.GetInputNameAllocated(0, alloc);
        auto out_name = det_session.GetOutputNameAllocated(0, alloc);
        const char* in_names[]  = {in_name.get()};
        const char* out_names[] = {out_name.get()};

        auto input_tensor = Ort::Value::CreateTensor<float>(
            mem, blob.data(), blob.size(), shape.data(), shape.size());

        auto outputs = det_session.Run(
            Ort::RunOptions{nullptr}, in_names, &input_tensor, 1, out_names, 1);

        auto out_shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
        int num_attrs = static_cast<int>(out_shape[1]);
        int num_preds = static_cast<int>(out_shape[2]);
        const float* raw = outputs[0].GetTensorData<float>();

        std::vector<cv::Rect>            cv_boxes;
        std::vector<float>               scores;
        std::vector<std::array<float,4>> raw_boxes;

        for (int i = 0; i < num_preds; ++i) {
            float best = 0.f;
            for (int c = 4; c < num_attrs; ++c)
                best = std::max(best, raw[c * num_preds + i]);
            if (best < min_conf) continue;

            float cx = raw[0 * num_preds + i];
            float cy = raw[1 * num_preds + i];
            float bw = raw[2 * num_preds + i];
            float bh = raw[3 * num_preds + i];

            float x1 = ((cx - bw / 2.f) - lb.dw) / lb.ratio;
            float y1 = ((cy - bh / 2.f) - lb.dh) / lb.ratio;
            float x2 = ((cx + bw / 2.f) - lb.dw) / lb.ratio;
            float y2 = ((cy + bh / 2.f) - lb.dh) / lb.ratio;

            raw_boxes.push_back({x1, y1, x2, y2});
            cv_boxes.emplace_back(static_cast<int>(x1), static_cast<int>(y1),
                                  static_cast<int>(x2 - x1), static_cast<int>(y2 - y1));
            scores.push_back(best);
        }

        if (cv_boxes.empty()) return {};

        std::vector<int> indices;
        cv::dnn::NMSBoxes(cv_boxes, scores, 0.25f, 0.45f, indices);

        std::vector<RawDetection> results;
        for (int idx : indices) {
            results.push_back({
                std::max(0, static_cast<int>(raw_boxes[idx][0])),
                std::max(0, static_cast<int>(raw_boxes[idx][1])),
                std::min(image.cols, static_cast<int>(raw_boxes[idx][2])),
                std::min(image.rows, static_cast<int>(raw_boxes[idx][3])),
                scores[idx]
            });
        }
        return results;
    }

    std::pair<std::string, float> run_ocr(const cv::Mat& plate_img)
    {
        cv::Mat plate = plate_img;
        if (plate.cols != 320) {
            float sc = 320.f / plate.cols;
            cv::resize(plate, plate, {320, static_cast<int>(plate.rows * sc)});
        }

        auto blob = ocr_preprocess(plate);
        std::vector<int64_t> shape = {1, 64, 128, 3};

        Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::AllocatorWithDefaultOptions alloc;

        auto in_name  = ocr_session.GetInputNameAllocated(0, alloc);
        auto out_name = ocr_session.GetOutputNameAllocated(0, alloc);
        const char* in_names[]  = {in_name.get()};
        const char* out_names[] = {out_name.get()};

        auto input_tensor = Ort::Value::CreateTensor<uint8_t>(
            mem, blob.data(), blob.size(), shape.data(), shape.size());

        auto outputs = ocr_session.Run(
            Ort::RunOptions{nullptr}, in_names, &input_tensor, 1, out_names, 1);

        auto out_shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
        int T           = static_cast<int>(out_shape[1]);
        int num_classes = static_cast<int>(out_shape[2]);

        return ctc_decode(outputs[0].GetTensorData<float>(), T, num_classes);
    }
};

LprEngine::LprEngine()
    : impl_(std::make_unique<Impl>())
{}

LprEngine::~LprEngine() = default;

std::vector<LprResult> LprEngine::process(const unsigned char* frame,
                                           int width, int height,
                                           float confidence)
{
    cv::Mat image(height, width, CV_8UC3, const_cast<unsigned char*>(frame));

    auto detections = impl_->run_detector(image, confidence);

    std::vector<LprResult> results;
    for (auto& d : detections) {
        cv::Mat crop = image(cv::Rect(d.x1, d.y1,
                                      d.x2 - d.x1,
                                      d.y2 - d.y1)).clone();
        auto [text, conf] = impl_->run_ocr(crop);
        results.push_back({text, conf, d.x1, d.y1, d.x2, d.y2});
    }
    return results;
}