from ultralytics import YOLO
import shutil
from fast_plate_ocr import LicensePlateRecognizer
from huggingface_hub import hf_hub_download

hf_hub_download(
    repo_id="wh0am-i/yolov11x-BrPlate",
    filename="best.pt",
    local_dir="."
)

# downloaded from https://huggingface.co/wh0am-i/yolov11x-BrPlate
detector_model = YOLO("best.pt")
detector_model.export(format="onnx")

recognizer = LicensePlateRecognizer("cct-xs-v1-global-model")
ocr_model = recognizer.model
shutil.copy(ocr_model._model_path, "./")
