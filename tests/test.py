import ctypes
import cv2
import numpy as np

# load image
img = cv2.imread("dataset/DMS2H35.jpg")
height, width, _ = img.shape
buffer = img.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))

# prepare struct for sdk result
class LprDetection(ctypes.Structure):
    _fields_ = [
        ("plate", ctypes.c_char * 16),
        ("confidence", ctypes.c_float),
        ("x1", ctypes.c_int),
        ("y1", ctypes.c_int),
        ("x2", ctypes.c_int),
        ("y2", ctypes.c_int),
    ]

max_results = 10
results = (LprDetection * max_results)()

# run sdk
sdk = ctypes.CDLL('build/liblpr_sdk_shared.dylib')

# tipos corretos
sdk.lpr_create.restype = ctypes.c_void_p

sdk.lpr_process.argtypes = [
    ctypes.c_void_p,                     # engine
    ctypes.POINTER(ctypes.c_uint8),     # frame
    ctypes.c_int,                       # width
    ctypes.c_int,                       # height
    ctypes.POINTER(LprDetection),       # results
    ctypes.c_int                        # max_results
]
sdk.lpr_process.restype = ctypes.c_int

lpr = sdk.lpr_create()

count = sdk.lpr_process(
    lpr,
    buffer,
    width,
    height,
    results,
    10
)

for i in range(count):

    plate = results[i].plate.decode()

    print("Plate:", plate)
    print("Confidence:", results[i].confidence)

    print(
        "BBox:",
        results[i].x1,
        results[i].y1,
        results[i].x2,
        results[i].y2
    )