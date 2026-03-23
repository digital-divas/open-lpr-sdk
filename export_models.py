from ultralytics import YOLO
import shutil
from fast_plate_ocr import LicensePlateRecognizer
from huggingface_hub import hf_hub_download
import os


hf_hub_download(
    repo_id="ezequielramos/yolov11n-BrPlate",
    filename="best.pt",
    local_dir="."
)

# downloaded from https://huggingface.co/ezequielramos/yolov11n-BrPlate
detector_model = YOLO("best.pt")
detector_model.export(format="onnx")

yolo_model = YOLO("yolo11n.pt")
yolo_model.export(format="onnx")

recognizer = LicensePlateRecognizer('cct-xs-v1-global-model')
ocr_model = recognizer.model
shutil.copy(ocr_model._model_path, "./")

os.makedirs("models", exist_ok=True)

shutil.move("best.onnx", "models/whoami.onnx")
shutil.move("cct_xs_v1_global.onnx", "models/cct_xs_v1_global.onnx")
shutil.move("yolo11n.onnx", "models/yolo11n.onnx")

os.remove("best.pt")