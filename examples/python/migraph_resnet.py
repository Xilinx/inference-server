
# This example contains Python commands necessary to bring up
# the migraphx worker and run a Resnet50 classification model on some images.
# Then, it calls the migraphx library directly with the same model, and compares results.
#  The model file in external/artifacts is 
# not in the git repo. and must be fetched with git or with 
# #  proteus get
#
# Model source:
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz

import os

os.getenv("PROTEUS_ROOT")

import proteus
import proteus.clients
import onnx
import cv2
import numpy as np
import time

import json       # json for reading labels file
import migraphx   # migraphx for validation (do the same inference directly)

# The make_nxn and preprocess functions are based on an migraphx example at 
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb 
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.

''' crop an image to square and then resize to desired dimension n x n'''
def make_nxn(image, n):
    width  = image.shape[1]
    height  = image.shape[0]
    if height > width:
        dif = height - width
        bar = dif // 2 
        square = image[(bar + (dif % 2)):(height - bar),:]
        return cv2.resize(square, (n, n))
    elif width > height:
        dif = width - height
        bar = dif // 2
        square = image[:,(bar + (dif % 2)):(width - bar)]
        return cv2.resize(square, (n, n))
    else:
        return cv2.resize(image, (n, n))
        
'''# Normalize array values to the data type, mean and std. dev. required by Resnet50
# img_data: numpy array in 3 dimensions [channels, rows, cols] with value range 0-255'''
def preprocess(img_data):
    # todo: are these vectors based on RGB images or BGR?  Results seem good
    mean_vec = np.array([0.485, 0.456, 0.406])
    stddev_vec = np.array([0.229, 0.224, 0.225])
    norm_img_data = np.zeros(img_data.shape).astype('float32')
    for i in range(img_data.shape[0]):  
        norm_img_data[i,:,:] = (img_data[i,:,:]/255 - mean_vec[i]) / stddev_vec[i]
    return norm_img_data

modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx"
# modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50-v1-12/resnet50-v1-12.onnx"

imagename = r"/workspace/proteus/external/artifacts/migraphx/yflower.jpg"        # a sunflower
imagename2 =r"/workspace/proteus/external/artifacts/migraphx/classification.jpg" # a dog

validation_dir = r"/workspace/proteus/external/artifacts/migraphx/ILSVRC2012_img_val/"   # imagenet validation images

#  load the onnx model to find the input shape, see https://stackoverflow.com/questions/56734576/find-input-shape-from-onnx-file
#  This code is applicable to any onnx model, but for resnet50 the required shape could have been hardcoded:   [1, 3, 224, 224]
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

print('This model\'s shape of input image is ', shape)    
if len(shape) != 4:
    print('Unable to read the image dimensions from ', modelname, '.  Expecting a 4-value shape tensor.')
    exit(-1)

# Load a picture of a flower
img  = cv2.imread(imagename).astype("float32")
# Crop to a square, resize
img = make_nxn(img, shape[2])
#  Normalize values with values specific to Resnet50
img = img.transpose(2, 0, 1)
img = preprocess(img)
# expected dimensions at this point are (3, 224, 224).  Note that the Resnet model expects 
# the channel dimension (3) to come before the rows and columns, but OpenCV places channels
# in the last dimension.

client = proteus.clients.HttpClient("http://127.0.0.1:8998")
print('waiting for server...',end='')
# call to initialize() or initializeLogging() is necessary before trying to contact the server.
# At time of writing, it's needed whether or not user asks for logging
proteus.initializeLogging()
while not client.serverLive():
    time.sleep(1)
print('ok.')

# +load worker.  The only parameter the migraphx worker requires is the model file name.
# It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
# it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
# future use.  It will read
# the array dimensions and data type from the model.

parameters = proteus.RequestParameters()
parameters.put("model", modelname)
# this call requests the server to either find a running instance of the named
# worker type, or else create one and initialize it with the parameters.  It's
# currently not possible to search for a worker by specifying the model you want.
worker_name = client.modelLoad("Migraphx", parameters)

# load the labels
with open('/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/imagenet_simple_labels.json') as json_data:
    labels = json.load(json_data)

# wait for the worker to load and compile model
ready = False
while not ready:
    try:
        ready = client.modelReady(worker_name)
    except ValueError:
        pass

files = os.listdir(validation_dir)
batch_size = 10
images = [None] * batch_size
index = 0
for file in files:
    filename = os.path.join(validation_dir, file)
    if os.path.isfile(filename):
        # Load a picture
        imgV  = cv2.imread(filename).astype("float32")
        imgV = make_nxn(imgV, shape[2])
        #  Normalize values with values specific to Resnet50
        imgV = imgV.transpose(2, 0, 1)
        imgV = preprocess(imgV)
        images[index % batch_size] = imgV
        index = index + 1
        if (index % batch_size == 0):
            print('processed this many: ', index)
            print("Creating inference request...", len(images))
            request = proteus.ImageInferenceRequest(images, False)
            print("request is ready")
            response = client.modelInfer(worker_name, request)
            assert not response.isError(), response.getError()
            print('Client received inference reply.')
            for output in response.getOutputs():
                assert output.datatype == proteus.DataType.FP32
                recv_data = output.getFp32Data()
                # the predicted category is the one with the highest match value
                answer = np.argmax(recv_data)
                print('client\'s analysis of result: best match category is ', answer,'  match value is ', recv_data[answer], '.  This is a picture of a ', labels[answer])
            






# # for debug: redisplay the processed images
display_img = images[5]
display_img = display_img.transpose(1, 2, 0)

# If we called proteus.initialize() earlier, a terminate() call is needed now or else we'll get an exception.
# proteus.terminate()
print('Done')

# If this line is commented out, worker persists with doRun thread active, and the entire script 
# can be run again without any reloading taking place
# client.unload('Migraphx')


