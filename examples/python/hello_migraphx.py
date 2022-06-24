
# This example contains Python commands necessary to bring up
# the migraphx worker and run a Resnet50 classification model on some images.
# Then, it calls the migraphx library directly with the same model, and compares results.
#  The model file in external/artifacts is 
# not in the git repo. and must be fetched with git or with 
# #  proteus get

# To get root access to the gpu device, invoke this script with
#  sudo PROTEUS_ROOT=/workspace/proteus python3 examples/python/hello_migraphx.py
#

from optparse import Values
import os

from attr import asdict
os.getenv("PROTEUS_ROOT")

import proteus
import onnx
import cv2
import math
import numpy as np

import json       # json for reading labels file
import migraphx   # migraphx for validation (do the same inference directly)

# The make_nxn and preprocess functions are based on an migraphx example at 
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb 
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.
# Note that since there is no standardized implementation of resize, etc., results and therefore
# preprocessing is not deterministic, validation set scores cannot be guaranteed to be replicated exactly.

# crop an image to square and then resize to desired dimension
def make_nxn(image, n):
    print('shape in nxn ', image.shape)
    width  = image.shape[1]
    height  = image.shape[0]
    if height > width:
        dif = height - width
        bar = dif // 2 
        square = image[(bar + (dif % 2)):(height - bar),:]
        print('square is--+ ', square.shape)
        return cv2.resize(square, (n, n))
    elif width > height:
        dif = width - height
        bar = dif // 2
        square = image[:,(bar + (dif % 2)):(width - bar)]
        print('square is--- ', square.shape)
        return cv2.resize(square, (n, n))
    else:
        return cv2.resize(image, (n, n))
        
# Normalize array values to the data type, mean and std. dev. required by Resnet50
# img_data: numpy array in 3 dimensions [channels, rows, cols] with value range 0-255
def preprocess(img_data):
    print('start preprocess with shape ', img_data.shape)
    mean_vec = np.array([0.485, 0.456, 0.406])
    stddev_vec = np.array([0.229, 0.224, 0.225])
    norm_img_data = np.zeros(img_data.shape).astype('float32')
    for i in range(img_data.shape[0]):  
        norm_img_data[i,:,:] = (img_data[i,:,:]/255 - mean_vec[i]) / stddev_vec[i]
    print('end preprocess with shape ', norm_img_data.shape)
    return norm_img_data

modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx"
# modelname = r"/workspace/proteus/external/artifacts/migraphx/resnet50-v1-12/resnet50-v1-12.onnx"
# imagename = r"/workspace/proteus/external/artifacts/migraphx/JG-COMP-HERO-UKRAINE-SOILDER.jpg"
imagename = r"/workspace/proteus/external/artifacts/migraphx/yflower.jpg"
imagename2 =r"/workspace/proteus/external/artifacts/migraphx/classification.jpg" # a dog

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

print('needed shape of input image is ', shape)    

# Read in the input, and resize it to fit the onnx model
# we expect a dim_value of 4
input_img = cv2.imread(imagename)

# Resnet50 model requires inputs of data type float32, range 0-1.0
# input_img = input_img.astype("float32")/255.
input_img = input_img.astype("float32")
img = input_img
print('shape of input image is ', img.shape)    

if len(shape) == 4:
    img = make_nxn(img, shape[2])
    #  Normalize values with values specific to Resnet50.  It's necessary to reorder axes
    # with transpose() because the preprocess fcn. is written to expect the color channels
    # in the first (0th) tensor axis which is the migraphx convention.
    img = img.transpose(2, 0, 1)
    img = preprocess(img)
    img = img.transpose(1,2,0)
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
img2  = cv2.imread(imagename2).astype("float32")
# img2 = cv2.resize(img2, shape[2:4])
img2 = make_nxn(img2, shape[2])
#  Normalize values with values specific to Resnet50
img2 = img2.transpose(2, 0, 1)
img2 = preprocess(img2)
img2 = img2.transpose(1, 2, 0)

# for debug: redisplay the processed images
x = np.max(img) - np.min(img)
renormalized_img = (img - np.min(img))*255/x
cv2.imwrite('sample1.jpg', renormalized_img.astype(np.uint8))
x2 = np.max(img2) - np.min(img2)
renormalized_img2 = (img2 - np.min(img2))*255/x2
cv2.imwrite('sample2.jpg',  renormalized_img2.astype(np.uint8))

# todo: creating an inference request with 2 images does not work correctly.  Both requests end up
# with the second image buffer address when read by the migraphx worker.
images=[img, img2]

print("Creating inference request...")
request = proteus.ImageInferenceRequest(images, False)

response = client.infer(worker_name, request)
assert not response.error, response.error_msg

print('Client received inference reply.')

# load the labels
with open('/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/imagenet_simple_labels.json') as json_data:
    labels = json.load(json_data)

for output in response.outputs:
    # output.data is a list of floats.  output also has a datatype member.
    data = output.data
    print('name of result returned by server is ', output.name)  # resnet model has only 1 output which doesn't have a name
    # the predicted category is the one with the highest match value
    answer = np.argmax(data)
    print('client\'s analysis of result: best match category is ', answer,'  match value is ', data[answer])
    print('This is a picture of a ', labels[answer])

#  Do a migraphx call directly for one image; compare results

print('Beginning comparison run-->parsing the model for migraphx...')
model = migraphx.parse_onnx(modelname)
model.compile(migraphx.get_target("gpu"))
# model.print()     # Printed in terminal.  Verbose; 351 lines of output.

# Reprocess the image to meet the shape requirements of migraphx
cropped_img = make_nxn(input_img, shape[2])
# put the last dimension (channels) first, expected by migraphx
new_img = cropped_img.transpose(2, 0, 1)
# normalize
test_img = preprocess(new_img)
# add a 4th tensor dimension in first dimension, expected by migraphx
test_img = np.expand_dims(test_img.astype('float32'),0)

# let's make sure the image wasn't corrupted by all this reshaping
# Put the channels dimension last again to view; value renormalization by 255. is approximate
tx = test_img[0].transpose(1,2,0)* 255.
print('tx=', type(tx), tx.shape)

# Run the model
results = model.run({'data': test_img})
# Extract the index of the top prediction
res_npa = np.array(results[0])
print ('category reported by migraphx is ', np.argmax(res_npa),'. This is a picture of a ', labels[np.argmax(res_npa)])

print('Done')

# If this line is commented out, client persists and the entire script can be run again without any reloading taking place
# client.unload('Migraphx')

# Model source:
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz


# sudo CXX=/opt/rocm/llvm/bin/clang++ cmake .. -DCMAKE_BUILD_TYPE=Release -DMIGRAPHX_ENABLE_PYTHON=On -DCMAKE_PREFIX_PATH=/workspace/proteus/AMDMIGraphX/deps -DMIGRAPHX_ENABLE_CPU=Off -DMIGRAPHX_ENABLE_GPU=On


