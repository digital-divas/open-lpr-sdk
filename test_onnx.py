import cv2
import numpy as np
import onnxruntime as ort
import os

CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
BLANK = len(CHARS)

ocr = ort.InferenceSession("models/cct_xs_v1_global.onnx")
detector = ort.InferenceSession("models/whoami.onnx")
yolo = ort.InferenceSession("models/yolo11n.onnx")

def ocr_preprocess(img):
    h, w = img.shape[:2]

    target_w = 128
    target_h = 64

    scale = min(target_w / w, target_h / h)

    new_w = int(w * scale)
    new_h = int(h * scale)

    resized = cv2.resize(img, (new_w, new_h))

    canvas = np.zeros((target_h, target_w, 3), dtype=np.uint8)

    x = (target_w - new_w) // 2
    y = (target_h - new_h) // 2

    canvas[y:y+new_h, x:x+new_w] = resized

    canvas = cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB)

    canvas = canvas.astype(np.uint8)

    canvas = canvas[np.newaxis]

    return canvas
    
def decode(output):

    plate = ""
    confs = []

    for timestep in output:
        idx = int(np.argmax(timestep))
        conf = float(np.max(timestep))

        if CHARS[idx] == "_":
            continue

        plate += CHARS[idx]
        confs.append(conf)

    confidence = np.mean(confs) if confs else 0

    return plate, confidence

def ocr_processor(image)->str:
    # image = cv2.imread(image_path)
    plate = ocr_preprocess(image)

    input_name = ocr.get_inputs()[0].name
    output = ocr.run(None, {input_name: plate})[0][0]

    return decode(output)

def letterbox(img, new_shape=(640,640), color=(114,114,114)):

    h, w = img.shape[:2]

    r = min(new_shape[0]/h, new_shape[1]/w)

    new_unpad = (int(round(w*r)), int(round(h*r)))

    img_resized = cv2.resize(img, new_unpad, interpolation=cv2.INTER_LINEAR)

    dw = new_shape[1] - new_unpad[0]
    dh = new_shape[0] - new_unpad[1]

    dw /= 2
    dh /= 2

    top = int(round(dh - 0.1))
    bottom = int(round(dh + 0.1))
    left = int(round(dw - 0.1))
    right = int(round(dw + 0.1))

    img_padded = cv2.copyMakeBorder(
        img_resized,
        top,
        bottom,
        left,
        right,
        cv2.BORDER_CONSTANT,
        value=color
    )

    return img_padded, r, (dw, dh)

def detect_yolo_onnx(image, session):
    img, ratio, (dw, dh) = letterbox(image, (320, 320))

    img = img.transpose(2,0,1)
    img = np.expand_dims(img, axis=0).astype(np.float32) / 255.0

    input_name = session.get_inputs()[0].name
    outputs = session.run(None, {input_name: img})

    return outputs, ratio, dw, dh

def parse_yolo(outputs, image, ratio, dw, dh, conf_thres=0.25):

    pred = outputs[0]

    if pred.shape[1] < pred.shape[2]:
        pred = pred[0].transpose(1,0)
    else:
        pred = pred[0]

    results = []
    boxes = []
    scores = []
    vehicle_classes = [1,2,3,5,7]  # bicycle, car, motorcycle, bus, truck

    for det in pred:

        x, y, w, h = det[:4]

        class_scores = det[4:]
        cls = int(np.argmax(class_scores))
        
        if cls not in vehicle_classes:
            continue
        
        conf = float(class_scores[cls])

        if conf < conf_thres:
            continue

        x1 = x - w/2
        y1 = y - h/2
        x2 = x + w/2
        y2 = y + h/2

        # desfaz letterbox
        x1 = (x1 - dw) / ratio
        x2 = (x2 - dw) / ratio
        y1 = (y1 - dh) / ratio
        y2 = (y2 - dh) / ratio

        x1 = int(max(0, x1))
        y1 = int(max(0, y1))
        x2 = int(min(image.shape[1], x2))
        y2 = int(min(image.shape[0], y2))

        boxes.append([x1,y1,x2,y2])
        scores.append(float(conf))

    boxes = np.array(boxes)
    scores = np.array(scores)

    if len(boxes) == 0:
        return []


    indices = cv2.dnn.NMSBoxes(
        boxes.tolist(),
        scores.tolist(),
        score_threshold=0.25,
        nms_threshold=0.45
    )

    results = []

    if len(indices) > 0:
        for i in indices.flatten():

            x1,y1,x2,y2 = boxes[i]

            x1 = int(max(0, x1))
            y1 = int(max(0, y1))
            x2 = int(min(image.shape[1], x2))
            y2 = int(min(image.shape[0], y2))

            results.append((x1,y1,x2,y2,scores[i]))

    return results

def detect_onnx(image, session):

    img, ratio, (dw, dh) = letterbox(image, (320, 320))

    img = img.transpose(2,0,1)
    img = np.expand_dims(img, axis=0).astype(np.float32) / 255.0

    input_name = session.get_inputs()[0].name

    outputs = session.run(None, {input_name: img})

    return outputs, ratio, dw, dh

def parse_yolo_output(outputs, image, ratio, dw, dh, min_confidence=0.25):

    pred = outputs[0][0]
    pred = pred.transpose(1,0) 

    conf_values = pred[:,4] * pred[:,5]

    boxes = []
    scores = []

    for det in pred:

        x, y, w, h = det[:4]

        class_scores = det[4:]
        cls = np.argmax(class_scores)
        conf = class_scores[cls]

        if conf < min_confidence:
            continue

        x1 = x - w/2
        y1 = y - h/2
        x2 = x + w/2
        y2 = y + h/2

        # desfazer letterbox
        x1 = (x1 - dw) / ratio
        x2 = (x2 - dw) / ratio
        y1 = (y1 - dh) / ratio
        y2 = (y2 - dh) / ratio

        boxes.append([x1,y1,x2,y2])
        scores.append(float(conf))

    boxes = np.array(boxes)
    scores = np.array(scores)

    if len(boxes) == 0:
        return []


    indices = cv2.dnn.NMSBoxes(
        boxes.tolist(),
        scores.tolist(),
        score_threshold=0.25,
        nms_threshold=0.45
    )

    results = []

    if len(indices) > 0:
        for i in indices.flatten():

            x1,y1,x2,y2 = boxes[i]

            x1 = int(max(0, x1))
            y1 = int(max(0, y1))
            x2 = int(min(image.shape[1], x2))
            y2 = int(min(image.shape[0], y2))

            results.append((x1,y1,x2,y2,scores[i], cls))

    return results

# for image_path in os.listdir('dataset'):
for image_path in ["TCE2D05.jpg", "SDQ2F48.jpg", "FWA5913.jpg"]:
    print('-----')
    print(f"dataset/{image_path}")
    image = cv2.imread(f"dataset/{image_path}")

    outputs, ratio, dw, dh = detect_yolo_onnx(image, yolo)
    yolo_detections = parse_yolo(outputs, image, ratio, dw, dh)
    
    print('yolo_detections',yolo_detections)

    detections = []

    if len(yolo_detections) > 0:
        for i in range(len(yolo_detections)):

            if (i > 3):
                break

            x1,y1,x2,y2,conf = yolo_detections[i]
            # print("VEHICLE:", conf)
            cv2.rectangle(image,(x1,y1),(x2,y2),(0,255,0),2)

            cv2.imshow('lixo',image)
            cv2.waitKey(0)

            h_img, w_img = image.shape[:2]

            margin = 0

            w = x2 - x1
            h = y2 - y1

            mx = int(w * margin)
            my = int(h * margin)

            cx1 = max(0, x1 - mx)
            cy1 = max(0, y1 - my)
            cx2 = min(w_img, x2 + mx)
            cy2 = min(h_img, y2 + my)

            crop = image[cy1:cy2, cx1:cx2]

            outputs, ratio, dw, dh = detect_onnx(crop, detector)


            internal_detections = parse_yolo_output(outputs, crop, ratio, dw, dh, 0.75)
            for detection in internal_detections:
                detection += (cx1,cy1,)
                detections.append(detection)

    else:
        print('nao achei veiculo, vou procurar placa')
        outputs, ratio, dw, dh = detect_onnx(image, detector)

        internal_detections = parse_yolo_output(outputs, image, ratio, dw, dh, 0.75)
        for detection in internal_detections:
            detection += (0,0,)
            detections.append(detection)

    print('detections',detections)

    for x1,y1,x2,y2,conf,class_id,cx1,cy1 in detections:
        x1 += cx1
        y1 += cy1
        x2 += cx1
        y2 += cy1

        plate = image[y1:y2, x1:x2]

        h, w = plate.shape[:2]

        target_width = 320
        scale = target_width / w

        new_w = int(w * scale)
        new_h = int(h * scale)

        plate = cv2.resize(plate, (new_w, new_h))
        print(plate.shape)

        ocr_output = ocr_processor(plate)
        print(ocr_output)

        cv2.rectangle(image,(x1,y1),(x2,y2),(0,255,0),2)
        cv2.imshow('lixo',image)
        cv2.waitKey(0)
