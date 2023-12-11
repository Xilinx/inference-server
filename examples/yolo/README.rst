..
    Copyright 2022 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

YOLO
----

This example demonstrates how to make an inference using a YOLO model with the MIGraphX backend in the inference server.
Install the requirements to run this example with ``pip install -r requirements.txt``
Use ``--help`` to see the available flags.

Introduction
^^^^^^^^^^^^

YOLO is an object detection model that accepts a single input tensor that contains a single image.
For this image, it returns three output tensors that can be postprocessed to derive bounding boxes and labels for what objects were detected.
The types of objects that may be detected depends on the dataset that the network is trained on.
In these examples, the default model is a YOLOv4 and is trained on the Coco dataset and has 80 output classes.
The output shapes are: (1, 52, 52, 3, 85) (1, 26, 26, 3, 85) (1, 13, 13, 3, 85).
There are 3 output layers. For each layer, there are 255 outputs: 85 values per anchor, times 3 anchors.
The 85 values of each anchor consists of 4 box coordinates describing the predicted bounding box (x, y, h, w), 1 object confidence, and 80 class confidences.

For more information about this model, look at the `ONNX model <https://github.com/onnx/models/tree/5faef4c33eba0395177850e1e31c4a6a9e634c82/vision/object_detection_segmentation/yolov4>`__ online.

Files
^^^^^

The files in this set of examples are:

::

    examples/yolo
    ├── migraphx.py              - MIGraphX example for YOLO
    ├── requirements.txt         - Python requirements for running YOLO examples
    ├── yolo_image_processing.py - pre- and post-processing for YOLO
    └── yolo.py                  - common functions for all YOLO examples
