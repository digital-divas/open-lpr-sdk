from ultralytics import YOLO
import shutil
from fast_plate_ocr import LicensePlateRecognizer

# downloaded from https://huggingface.co/wh0am-i/yolov11x-BrPlate
detector_model = YOLO("whoami.pt")
detector_model.export(format="onnx")

recognizer = LicensePlateRecognizer('cct-xs-v1-global-model')
ocr_model = recognizer.model
shutil.copy(ocr_model._model_path, "./")