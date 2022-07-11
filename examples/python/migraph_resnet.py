# This example brings up
# the migraphx worker and runs a Resnet50 classification model on the official imagenet validation set (50,000 images).
#
#  The model file and test data is
# not in the git repo. and must be fetched with git or
# #  proteus get or wget
#
# Model source:
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz

# Source of ground truth labels for validation set:   ILSVRC2012_validation_ground_truth.txt
# https://github.com/mvermeulen/rocm-migraphx/tree/master/datasets/imagenet/val.txt

import os

os.getenv("PROTEUS_ROOT")

import proteus
import proteus.clients
import argparse
import onnx
import cv2
import numpy as np
import time

import json  # json for reading labels file
import migraphx  # migraphx for validation (do the same inference directly)

# The make_nxn and preprocess functions are based on an migraphx example at
# AMDMIGraphx/examples/vision/python_resnet50/resnet50_inference.ipynb
# The mean and standard dev. values used for this normalization are requirements of the Resnet50 model.

""" crop an image to square and then resize to desired dimension n x n"""


def make_nxn(image, n):
    width = image.shape[1]
    height = image.shape[0]
    if height > width:
        dif = height - width
        bar = dif // 2
        square = image[(bar + (dif % 2)) : (height - bar), :]
        return cv2.resize(square, (n, n))
    elif width > height:
        dif = width - height
        bar = dif // 2
        square = image[:, (bar + (dif % 2)) : (width - bar)]
        return cv2.resize(square, (n, n))
    else:
        return cv2.resize(image, (n, n))


"""# Normalize array values to the data type, mean and std. dev. required by Resnet50
# img_data: numpy array in 3 dimensions [channels, rows, cols] with value range 0-255"""


def preprocess(img_data):
    # todo: are these vectors based on RGB images or BGR?  Results seem good
    mean_vec = np.array([0.485, 0.456, 0.406])
    stddev_vec = np.array([0.229, 0.224, 0.225])
    norm_img_data = np.zeros(img_data.shape).astype("float32")
    for i in range(img_data.shape[0]):
        norm_img_data[i, :, :] = (img_data[i, :, :] / 255 - mean_vec[i]) / stddev_vec[i]
    return norm_img_data


#
# Read command line arguments
#
root = os.getenv("PROTEUS_ROOT")
assert root is not None

# Get the arguments required from the user
parser = argparse.ArgumentParser(
    description="Validation (working) for Proteus Migraphx worker"
)
parser.add_argument(
    "--request_size",
    "-r",
    type=int,
    required=False,
    help="Number of images per REST request",
    default=4,
)

parser.add_argument(
    "--batch_size",
    "-b",
    type=int,
    required=False,
    default=64,
    help="Batch size for migraphx evaluation. Default is 64. " "(currently not used)",
)
parser.add_argument(
    "--modelfile",
    "-m",
    type=str,
    required=False,
    default=os.path.join(
        root, r"external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx"
    ),
    help="Location of model file on server",
)
parser.add_argument(
    "--validation_dir",
    "-v",
    type=str,
    required=False,
    default=os.path.join(root, r"external/artifacts/migraphx/ILSVRC2012_img_val"),
    help="The directory containing validation images, such as the Imagenet validation set if available",
)
parser.add_argument(
    "--groundtruth",
    "-g",
    type=str,
    required=False,
    default=r"val.txt",
    help="The list of correct category results (ground truth) for the validation set; file must be in the validation directory",
)

parser.add_argument(
    "--labels",
    "-l",
    type=str,
    required=False,
    default=os.path.join(
        root, r"tests/assets/imagenet_simple_labels.json"
    ),
    help="The file containing label names for the model's categories",
)

# Parse arguments
args = parser.parse_args()
batch_size = args.request_size  # NOT args.batch_size!
modelname = args.modelfile
validation_dir = args.validation_dir
validation_answers_file = os.path.join(validation_dir, args.groundtruth)
labels_file = args.labels

#
#           End read command line arguments
#

if not os.path.exists(validation_dir):
    print(f"{validation_dir} does not exist, skipping migraphx validation test")
    # exiting with zero to prevent failing automated tests
    exit(0)

#  load the onnx model to find the input shape, see https://stackoverflow.com/questions/56734576/find-input-shape-from-onnx-file
#  We assume here that client is on the same file system as the server
#  This code is applicable to any onnx model, but for resnet50 the required shape could have been hardcoded:   [1, 3, 224, 224]
shape = []
model = onnx.load(modelname)
for input in model.graph.input:
    if input.name == "data":
        print(input.name, end=": ")
        # get type of input tensor
        tensor_type = input.type.tensor_type
        # check if it has a shape:
        if tensor_type.HasField("shape"):
            # iterate through dimensions of the shape:
            for d in tensor_type.shape.dim:
                # the dimension may have a definite (integer) value or a symbolic identifier or neither:
                if d.HasField("dim_value"):
                    shape.append(d.dim_value)
        else:
            print("unknown rank", end="")
        print()
        break

print("This model's shape of input image is ", shape)
if len(shape) != 4:
    print(
        "Unable to read the image dimensions from ",
        modelname,
        ".  Expecting a 4-value shape tensor.",
    )
    exit(-1)


client = proteus.clients.HttpClient("http://127.0.0.1:8998")
print("waiting for server...", end="")
# call to initialize() or initializeLogging() is necessary before trying to contact the server.
# At time of writing, it's needed whether or not user asks for logging
proteus.initializeLogging()
while not client.serverLive():
    time.sleep(1)
print("ok.")

# +load worker.  The only parameter the migraphx worker requires is the model file name.
# It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
# it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
# future use.  It will read
# the array dimensions and data type from the model.

parameters = proteus.RequestParameters()
parameters.put("model", modelname)
# this call requests the server to either find a running instance of the named
# worker type, or else create one and initialize it with the parameters.
worker_name = client.workerLoad("Migraphx", parameters)

# load the labels
with open(labels_file, "r") as json_data:
    labels = json.load(json_data)

# wait for the worker to load and compile model
ready = False
while not ready:
    try:
        ready = client.modelReady(worker_name)
    except ValueError:
        pass

# list all the *.jpg images in the validation image directory
files = os.listdir(validation_dir)
files.sort()
files.remove("val.txt")

# Read the "answers" file
ground_truth = [None] * len(files)
i = 1
with open(validation_answers_file, "r") as labelfile:
    while i < len(ground_truth):
        ground_truth[i] = labelfile.readline().split()[1]
        i = i + 1

#
#   Loop thru the image set, reading images and putting together inference requests in batches.
#   Then save the results and compare with ground truth
#
results = [None] * len(files)
correct_answers = 0
wrong_answers = 0

images = [None] * batch_size
index = 0
for file in files:
    filename = os.path.join(validation_dir, file)
    if os.path.isfile(filename) and filename.find(".JPEG") != -1:
        print("file ", file)
        # Load a picture
        imgV = cv2.imread(filename).astype("float32")
        imgV = cv2.cvtColor(imgV, cv2.COLOR_BGR2RGB)
        imgV = make_nxn(imgV, shape[2])
        #  Normalize values with values specific to Resnet50
        imgV = imgV.transpose(2, 0, 1)
        imgV = preprocess(imgV)
        images[index % batch_size] = imgV
        index = index + 1
        if index % batch_size == 0:
            print("processed so far: ", index, " of ", len(files))
            print("Creating inference request with ", len(images), " items")
            request = proteus.ImageInferenceRequest(images, False)
            print("request is ready.  Sending...")
            response = client.modelInfer(worker_name, request)
            assert not response.isError(), response.getError()
            print("Client received inference reply.")
            j = 0
            for output in response.getOutputs():
                assert output.datatype == proteus.DataType.FP32
                recv_data = output.getFp32Data()
                # the predicted category is the one with the highest match value
                answer = np.argmax(recv_data)
                step_index = index - batch_size + j + 1
                print(
                    "Reading result: best match category is ",
                    answer,
                    "  value is ",
                    recv_data[answer],
                    ".  This is a picture of a ",
                    labels[answer],
                    "    ground truth: ",
                    ground_truth[step_index],
                )
                if int(answer) == int(ground_truth[step_index]):
                    correct_answers = correct_answers + 1.0
                else:
                    wrong_answers = wrong_answers + 1.0
                j = j + 1
            print(
                "Correct: ",
                correct_answers,
                "  Wrong: ",
                wrong_answers,
                "    Accuracy: ",
                correct_answers / (correct_answers + wrong_answers),
            )
            print("     ----------------------------------------")

# # for debug: redisplay the processed images
display_img = images[5]
display_img = display_img.transpose(1, 2, 0)

# If we called proteus.initialize() earlier, a terminate() call is needed now or else we'll get an exception.
# proteus.terminate()


print("Done")

# If this line is commented out, worker persists with doRun thread active, and the entire script
# can be run again without any reloading taking place
# client.unload('Migraphx')
