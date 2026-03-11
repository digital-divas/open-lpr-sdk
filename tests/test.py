import ctypes
import cv2
import numpy as np


class LprDetection(ctypes.Structure):
    _fields_ = [
        ("plate", ctypes.c_char * 16),
        ("confidence", ctypes.c_float),
        ("x1", ctypes.c_int),
        ("y1", ctypes.c_int),
        ("x2", ctypes.c_int),
        ("y2", ctypes.c_int),
    ]


sdk = ctypes.CDLL("build/liblpr.dylib")

sdk.lpr_create.restype = ctypes.c_void_p
sdk.lpr_create.argtypes = []

sdk.lpr_destroy.restype = None
sdk.lpr_destroy.argtypes = [ctypes.c_void_p]

sdk.lpr_process.restype = ctypes.c_int
sdk.lpr_process.argtypes = [
    ctypes.c_void_p,  # handle
    ctypes.POINTER(ctypes.c_uint8),  # frame
    ctypes.c_int,  # width
    ctypes.c_int,  # height
    ctypes.POINTER(LprDetection),  # out_results
    ctypes.c_int,  # max_results
]

img = cv2.imread("dataset/DMS2H35.jpg")
height, width, _ = img.shape
frame = np.ascontiguousarray(img)
buffer = frame.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))

lpr = sdk.lpr_create()

max_results = 10
results = (LprDetection * max_results)()
count = sdk.lpr_process(lpr, buffer, width, height, results, max_results)

for i in range(count):
    r = results[i]
    print("Plate:", r.plate.decode())
    print("Confidence:", r.confidence)
    print("BBox:", r.x1, r.y1, r.x2, r.y2)

sdk.lpr_destroy(lpr)
