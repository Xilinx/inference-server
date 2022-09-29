# Copyright 2022 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
This example contains Python commands necessary to bring up
the migraphx worker and run a Yolo classification model on some images.
See  https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/vision/python_yolov4/yolov4_inference.ipynb for details.


Outputs:  see https://github.com/onnx/models/tree/main/vision/object_detection_segmentation/yolov4 README

Output shapes: (1, 52, 52, 3, 85) (1, 26, 26, 3, 85) (1, 13, 13, 3, 85)

There are 3 output layers. For each layer, there are 255 outputs: 85 values per anchor, times 3 anchors.

The 85 values of each anchor consists of 4 box coordinates describing the predicted bounding box (x, y, h, w), 1 object confidence, and 80 class confidences.
"""
# import migraphx    # redundant with proteus
import cv2
import time
import numpy as np
from PIL import Image
# from inspect import getmembers, isfunction

import argparse
import os
import sys

# the more/utilities directory is a local, temporary location for dev.
sys.path.append('/workspace/proteus/external/artifacts/migraphx/more')
from utilities import image_processing as ip

import time

import cv2
import numpy as np

import proteus
import proteus.clients

import migraphx # debug
import onnxruntime as rt

# The make_nxn and preprocess functions are based on an migraphx example at
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.

def read_class_names(class_file_name):
    '''loads class name from a file'''
    names = {}
    with open(class_file_name, 'r') as data:
        for ID, name in enumerate(data):
            names[ID] = name.strip('\n')
    return names

def run_migraphx(model_name, img, labels):
    """
    Do a migraphx call directly for one image; compare results

    Args:
        model_name (str): path to the model
        img (np.array): image to send
        labels (dict): label strings for resnet
    """
    import migraphx

    print("Beginning comparison run-->parsing the model for migraphx...")
    model = migraphx.parse_onnx(model_name)
    model.compile(migraphx.get_target("gpu"))
    print("Ok.")

    # img and img2 are already the correct shape for the Inf Server, but need another dimension for Python API migraphx
    test_img = img
    # add a 4th tensor dimension in first position, expected by migraphx
    test_img = np.expand_dims(test_img, 0)

    # Run the inference
    results = model.run({"data": test_img})
    detections = results
    print("Output shape:", list(map(lambda detection: detection.shape, detections)))


def parse_args():
    """
    Read command line arguments
    """
    root = os.getenv("PROTEUS_ROOT")
    assert root is not None

    # Get the arguments required from the user
    parser = argparse.ArgumentParser(
        description="Example client Proteus Migraphx worker"
    )

    parser.add_argument(
        "--modelfile",
        "-m",
        type=str,
        required=False,
        default=os.path.join(
            root, "external/artifacts/migraphx/more/utilities/yolov4.onnx"
        ),
        help="Location of model file on server",
    )

    parser.add_argument(
        "--labels",
        "-l",
        type=str,
        required=False,
        default=os.path.join(root, "external/artifacts/migraphx/more/utilities/coco.names"),
        help="The file containing label names for the model's categories",
    )

    parser.add_argument(
        "--image",
        "-i",
        type=str,
        required=False,
        default=os.path.join(root, "external/artifacts/migraphx/more/utilities/input.jpg"),  # an outdoor crowd scene
        help="An image to try inference on.  Use git-lfs to pull image assets",
    )

    parser.add_argument(
        "--image2",
        "-g",
        type=str,
        required=False,
        default=os.path.join(
            root, "tests/assets/bicycle-384566_640.jpg"
        ),  # a bicyclist
        help="A second image to try inference on",
    )

    # Parse arguments
    return parser.parse_args()


def main(args):

    modelname = args.modelfile
    imagename = args.image
    imagename2 = args.image2
    labels_file = args.labels

    client = proteus.clients.HttpClient("http://127.0.0.1:8998")
    print("waiting for server...", end="")
    start_server = not client.serverLive()
    if start_server:
        server = proteus.servers.Server()
        server.startHttp(8998)
        while not client.serverLive():
            time.sleep(1)
    print("ok.")

    metadata = client.serverMetadata()

    if not "migraphx" in metadata.extensions:
        print("MIGraphX support required but not found.")
        sys.exit(0)

    input_size = 416

    original_image = cv2.imread(imagename)
    original_image = cv2.cvtColor(original_image, cv2.COLOR_BGR2RGB)
    original_image_size = original_image.shape[:2]

    img = ip.image_preprocess(np.copy(original_image), [input_size, input_size])
    img = img[np.newaxis, ...].astype(np.float32)

    # +load worker.  The only parameter the migraphx worker requires is the model file name.
    # batch and timeout are optional.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use.  It will read
    # the array dimensions and data type from the model.
    parameters = proteus.RequestParameters()
    parameters.put("model", modelname)

    # I found that allocation could fail with a large batch value of 64 and large (13) default buffer count in the migraphx worker
    # Beyond batch size 56, the worker seems to lock up while compiling the model
    parameters.put("batch", 1)
     
    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    worker_name = client.workerLoad("Migraphx", parameters)

    # wait for the worker to load and compile model
    ready = False
    while not ready:
        ready = client.modelReady(worker_name)

    #
    # create a multi-image inference request and send it
    #
    images = [img, img]

    print("Creating inference request set...")
    images = [proteus.ImageInferenceRequest(image) for image in images]
    responses = proteus.client_operators.inferAsyncOrdered(client, worker_name, images)  
    for response in responses:  
        assert not response.isError(), response.getError()

    print("Client received inference reply.")

    for it, response in enumerate(responses): 
        detections = []
        for out in response.getOutputs():
            assert out.datatype == proteus.DataType.FP32
            print('output shape is  ', out.shape)    
            this_detect = np.array(out.getFp32Data())
            newshape = out.shape
            # add a 0'th dimension of 1, to make 5.  (the migraphx worker stripped off the batch size.)
            newshape.insert(0,1)
            this_detect = this_detect.reshape(newshape)
            detections.append(this_detect)

    #
    # Post-process the model outputs and display image with detection bounding boxes
    #  
        ANCHORS = "external/artifacts/migraphx/more/utilities/yolov4_anchors.txt"
        STRIDES = [8, 16, 32]
        XYSCALE = [1.2, 1.1, 1.05]

        ANCHORS = ip.get_anchors(ANCHORS)
        STRIDES = np.array(STRIDES)

        pred_bbox = ip.postprocess_bbbox(detections, ANCHORS, STRIDES, XYSCALE)
        bboxes = ip.postprocess_boxes(pred_bbox, original_image_size, input_size, 0.25)
        bboxes = ip.nms(bboxes, 0.213, method='nms')
        image = ip.draw_bbox(original_image, bboxes, 'external/artifacts/migraphx/more/utilities/coco.names')

        image = Image.fromarray(image)
        output_name = "external/artifacts/migraphx/more/utilities/yolo4_output" + str(it) + ".jpg"
        image.save(output_name)

        print("Your marked-up image is at " + output_name)
    print("Done.")
    #
    #    Alan's related but not identical example is at https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/vision/python_yolov4/yolov4_inference.ipynb
    

if __name__ == "__main__":
    args = parse_args()
    main(args)
    