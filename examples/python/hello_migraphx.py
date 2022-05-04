
# This example contains the minimal Python commands necessary to bring up
# the migraphx worker and verify that it runs.  The model file in external/artifacts is 
# not in the git repo. and must be fetched with git or with 
# #  proteus get

# To get root access to the gpu device, invoke this script with
#  sudo PROTEUS_ROOT=/workspace/proteus python3 examples/python/hello_migraphx.py
#

import os
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

modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50v2/resnet50-v2-7.onnx"
imagename = r"/workspace/proteus/external/artifacts/migraphx/JG-COMP-HERO-UKRAINE-SOILDER.jpg"

#  load the onnx model to find the input shape
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
                    print ('value ', d.dim_value, end=", ")  # known dimension
                elif (d.HasField("dim_param")):
                    print ('param ', d.dim_param, end=", ")  # unknown dimension with symbolic name
                else:
                    print ("?", end=", ")  # unknown dimension with no name
        else:
            print ("unknown rank", end="")
        print()
        break

print(shape)    

# Read in the input, and resize it to fit the onnx model
# we expect a dim_value of 4, and this script just fails if not
input_img = cv2.imread(imagename)
if len(shape) == 4:
    print('resizing ', input_img.shape, '!', shape[2:4], end="==>")
    img = cv2.resize(input_img, shape[2:4])
    print(img.shape, '!!')


server = proteus.Server()
client = proteus.RestClient("127.0.0.1:8998")
client.wait_until_live()

# +load worker:
parameters = {"model": modelname,
              "input": img.tolist()
			  }

response = client.load("Migraphx", parameters)

# send the image as a blob, not a path
# there's a helper that creates a request from an image, see example

# Model source:
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz
