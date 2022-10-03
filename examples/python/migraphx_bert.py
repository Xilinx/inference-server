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

#####################################################################################
# The MIT License (MIT)
#
# Copyright (c) 2015-2022 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#####################################################################################

'''
Bert model client

This example contains Python commands necessary to bring up
the migraphx worker and run a Bert language processing model.

Notes to self:
Read download instructions from Ted T. at https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/tree/develop/examples/nlp/python_bert_squad

cd external/artifacts/migraphx/bert_squad
pip3 install onnxruntime tokenizers==0.12.1

apt-get install unzip                        <== doesn't work in docker

wget https://github.com/onnx/models/raw/main/text/machine_comprehension/bert-squad/model/bertsquad-10.onnx
wget -q https://storage.googleapis.com/bert_models/2018_10_18/uncased_L-12_H-768_A-12.zip
unzip uncased_L-12_H-768_A-12.zip
rm uncased_L-12_H-768_A-12.zip

[in Docker]   pip3 install tokenizers

[in bash ]
cp /home/bpickrel/is-fork/inference-server/AMDMIGraphX/examples/nlp/python_bert_squad/requirements_bertsquad.txt  ~/is-fork/inference-server/external/artifacts/migraphx/bert_squad/
cp /home/bpickrel/is-fork/inference-server/AMDMIGraphX/examples/nlp/python_bert_squad/run_onnx_squad.py ~/is-fork/inference-server/external/artifacts/migraphx/bert_squad/
cp /home/bpickrel/is-fork/inference-server/AMDMIGraphX/examples/nlp/python_bert_squad/*.json  ~/is-fork/inference-server/external/artifacts/migraphx/bert_squad/

'''


import os
# The following packages aren't automatically installed in the dockerfile:
try:
    import tokenizers
except ImportError:
    os.system("pip3 install tokenizers==0.12.1")
    import tokenizers

try:
    import onnxruntime
except ImportError:
    os.system("pip3 install onnxruntime")
    import onnxruntime



import proteus
# from _proteus import *
import numpy as np
import json
import os.path
import argparse
import os
import sys
import time
import tokenizers
import collections
base_dir="external/artifacts/migraphx/bert_squad"
sys.path.append(base_dir)
from run_onnx_squad import (
    read_squad_examples,
    write_predictions,
    convert_examples_to_features,
)
# import migraphx


RawResult = collections.namedtuple(
    "RawResult", ["unique_id", "start_logits", "end_logits"]
)

#######################################
input_file = os.path.join(base_dir, "inputs_amd.json")
with open(input_file) as json_file:
    test_data = json.load(json_file)
    # print(json.dumps(test_data, indent=2))

# preprocess input
predict_file = os.path.join(base_dir, "inputs_amd.json")

# Use read_squad_examples method from run_onnx_squad to read the input file
eval_examples = read_squad_examples(input_file=predict_file)

max_seq_length = 256
doc_stride = 128
max_query_length = 64
batch_size = 13
n_best_size = 20
max_answer_length = 30

vocab_file = os.path.join(base_dir, os.path.join("uncased_L-12_H-768_A-12", "vocab.txt"))
tokenizer = tokenizers.BertWordPieceTokenizer(vocab_file)

# Use convert_examples_to_features method from run_onnx_squad to get parameters from the input
# the first 3 are lists of ndarray, extra_data is a list of Feature (a namedtuple)
input_ids, input_mask, segment_ids, extra_data = convert_examples_to_features(
    eval_examples, tokenizer, max_seq_length, doc_stride, max_query_length
)
print('line 115 convert yields  ', len(input_ids),'\n\n', input_ids[0].shape,'\n\n', input_mask[0].shape,'\n\n', segment_ids[0].shape,'\n\n')
# extra_data[0] is a Feature

#############################################################
#
#   Make Inference Server connection
#
#############################################################
client = proteus.clients.HttpClient("http://127.0.0.1:8998")
print("waiting for server...", end="")
start_server = not client.serverLive()
if start_server:
    server = proteus.servers.Server()
    server.startHttp(8998)
    while not client.serverLive():
        time.sleep(1)
print("ok.  Connected.")

metadata = client.serverMetadata()

if not "migraphx" in metadata.extensions:
    print("MIGraphX support required but not found.")
    sys.exit(0)

#############################################################
#
#   Load the model
#
#############################################################
# +load worker.  The only parameter the migraphx worker requires is the model file name.
# batch and timeout are optional.
# It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
# it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
# future use.  It will read
# the array dimensions and data type from the model.
modelname = os.path.join(base_dir, "bertsquad-10.onnx")
parameters = proteus.RequestParameters()
parameters.put("model", modelname)

parameters.put("batch", batch_size)
    
# this call requests the server to either find a running instance of the named
# worker type, or else create one and initialize it with the parameters.
print('Requesting worker load with model ', modelname, '...')
worker_name = client.workerLoad("Migraphx", parameters)
print("ok.  Loaded and parsed worker ", worker_name)

# wait for the worker to load and compile model
ready = False
while not ready:
    ready = client.modelReady(worker_name)

# If we get here, the worker is capable of loading multi inputs
print('Model loaded.')


n = len(input_ids)
bs = batch_size
all_results = []


# input_n.datatype = getattr(DataType, str(image.dtype).upper())
# input_n.shape = [*image.shape]  # Convert tuple to list
# _set_data(input_n, image.flatten())

print('eval_ex -------------- ', len(eval_examples), bs, n)
requests=[]
    
# For now, should only submit a whole batch worth of requests at once.  The Inference Server might 
# populate any dummies with junk data that would crash the worker

for idx in range(0,2):
    # Create an InferenceRequest
    request = proteus.predict_api.InferenceRequest()

    item = eval_examples[idx]   # class SquadExample

    # add items to inference request

    input_n = proteus.predict_api.InferenceRequestInput()
    input_n.name = f"input_ids:0"
    input_n.datatype = proteus.DataType.INT64
    input_n.shape = (1, 256,)
    # input_n.setInt64Data(input_ids[idx : idx + bs])
    input_n.setInt64Data(input_ids[idx : idx + bs])
    request.addInputTensor(input_n)

    input_n = proteus.predict_api.InferenceRequestInput()
    input_n.name = f"input_mask:0"
    input_n.datatype = proteus.DataType.INT64
    input_n.shape = (1, 256,)
    input_n.setInt64Data( input_mask[idx : idx + bs])
    request.addInputTensor(input_n)

    input_n = proteus.predict_api.InferenceRequestInput()
    input_n.name = f"segment_ids:0"
    input_n.datatype = proteus.DataType.INT64
    input_n.shape = (1, 256,)
    input_n.setInt64Data( segment_ids[idx : idx + bs])
    request.addInputTensor(input_n)

    # This comes from the first argument; I think it's supposed to be output names 
    #       see https://onnxruntime.ai/docs/api/python/api_summary.html#load-and-run-a-model
    # item = eval_examples[idx]   # class SquadExample
    input_n = proteus.predict_api.InferenceRequestInput()
    input_n.name = f"unique_ids_raw_output___9:0"
    input_n.datatype = proteus.DataType.INT64
    input_n.shape = (1,)
    input_n.setInt64Data( np.array([item.qas_id], dtype=np.int64))        
    request.addInputTensor(input_n)
    requests.append(request)

print('request batch is ready, size ', len(requests),  '.  Sending...')
responses = proteus.client_operators.inferAsyncOrdered(client, worker_name, requests)  
print('responses received. ')


output_dir = os.path.join(base_dir, "predictions")
os.makedirs(output_dir, exist_ok=True)
output_prediction_file = os.path.join(output_dir, "predictions.json")
output_nbest_file = os.path.join(output_dir, "nbest_predictions.json")
write_predictions(
    eval_examples,
    extra_data,
    all_results,
    n_best_size,
    max_answer_length,
    True,
    output_prediction_file,
    output_nbest_file,
)

with open(output_prediction_file) as json_file:
    test_data = json.load(json_file)
    print(json.dumps(test_data, indent=2))
