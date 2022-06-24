
# This example contains the minimal Python commands necessary to bring up
# the migraphx worker and verify that it runs.  The model file in external/artifacts is 
# not in the git repo. and must be fetched with git or with 
# #  proteus get

# To get root access to the gpu device, invoke this script with
#  sudo PROTEUS_ROOT=/workspace/proteus python3 examples/python/hello_migraphx.py
#

import os

from attr import asdict
os.getenv("PROTEUS_ROOT")

# workaround:  we don't know why the path isn't right when running this as root
# import sys
# sys.path=['/workspace/proteus/src/python/src', \
#     '/opt/xilinx/xrt/python', '/workspace/proteus', '/usr/lib/python36.zip', '/usr/lib/python3.6', \
#     '/usr/lib/python3.6/lib-dynload', '/home/proteus-user/.local/lib/python3.6/site-packages', \
#     '/usr/local/lib/python3.6/dist-packages', '/usr/lib/python3/dist-packages', '/opt/rocm/lib']

import proteus
import onnx
import cv2
import math
import numpy as np

modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx"
# modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50-v1-12/resnet50-v1-12.onnx"
# imagename = r"/workspace/proteus/external/artifacts/migraphx/JG-COMP-HERO-UKRAINE-SOILDER.jpg"
imagename = r"/workspace/proteus/external/artifacts/migraphx/yflower.jpg"
imagename2 =r"/workspace/proteus/external/artifacts/migraphx/classification.jpg" # a dog

#  load the onnx model to find the input shape, see https://stackoverflow.com/questions/56734576/find-input-shape-from-onnx-file
shape=[]
model = onnx.load(modelname)
for input in model.graph.input:
    if input.name == 'data':
        print (input.name, end=": ")
        # get type of input tensor
        tensor_type = input.type.tensor_type
        # check if it has a shape:
        if (tensor_type.HasField("shape")):
            # iterate through dimensions of the shape:
            for d in tensor_type.shape.dim:
                # the dimension may have a definite (integer) value or a symbolic identifier or neither:
                if (d.HasField("dim_value")):
                    shape.append(d.dim_value)
        else:
            print ("unknown rank", end="")
        print()
        break

print('shape of input image is ', shape)    

# Read in the input, and resize it to fit the onnx model
# we expect a dim_value of 4
input_img = cv2.imread(imagename)

# Resnet50 model requires inputs of data type float32, range 0-1.0
input_img = input_img.astype("float32")/255.

print('type of image is now ', input_img.dtype)

if len(shape) == 4:
    print('resizing ', input_img.shape, '!', shape[2:4], end="==>")
    img = cv2.resize(input_img, shape[2:4])
    # debug: make so small that contents is human readable
    # img = cv2.resize(input_img, [20,70])
    print('image shape is ', img.shape, '!!')
    # print('image as list is ', img.tolist())   # this can be very long
else:
    print('Unable to read the image dimensions from ', modelname, '.  Expecting a 4-value shape tensor.')
    exit(-1)


server = proteus.Server()
client = proteus.RestClient("127.0.0.1:8998")
print('waiting for server...',end='')
client.wait_until_live()
print('ok.')
# +load worker:     DEFINE A SET OF PARAMETERS THAT THE MIGRAPHX WORKER 
# NEEDS.  
# ANY KEY-VALUE PAIRS ARE LEGAL

parameters = {"model": modelname,
                # any other migraphx-specific parameters here
			  }

response = client.load("Migraphx", parameters)
assert not response.error, response.error_msg
worker_name = response.html  # Migraphx
print('response from load is', response.html)

print('image shape after resizing is ', img.shape)
rows,cols = img.shape[1:3]

# Load a picture of a dog
img2  = cv2.imread(imagename2).astype("float32")/255.
img2 = cv2.resize(img2, shape[2:4])

# for debug: rewrite the images
cv2.imwrite('sample1.jpg', (img*255).astype(np.uint8))
cv2.imwrite('sample2.jpg',  (img2*255).astype(np.uint8))

# todo: creating an inference request with 2 images does not work correctly.  Both requests end up
# with the second image buffer address when read by the migraphx worker.
images=[img, img2]

print("Creating inference request...")
request = proteus.ImageInferenceRequest(images, False)

response = client.infer(worker_name, request)
assert not response.error, response.error_msg

print('Client received inference reply.')
for output in response.outputs:
    # output.data is a list of floats.  output also has a datatype member.
    data = output.data
    print('name of result returned by server is ', output.name)  # resnet model has only 1 output which doesn't have a name
    # the predicted category is the one with the highest match value
    answer = np.argmax(data)
    print('client\'s analysis of result: best match category is ', answer,'  match value is ', data[answer])

    # todo: match this index with a category name for resnet50

print('Done')

# If this line is commented out, client persists and the entire script can be run again without any reloading taking place
# client.unload('Migraphx')

# Model source:
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz
