# MIT License
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

"""
This example demonstrates how you can use the MIGraphX backend to run inference
on an AMD GPU with a BERT ONNX model.

This example is based on a similar example in MIGraphX:
https://github.com/ROCmSoftwarePlatform/AMDMIGraphX/blob/develop/examples/nlp/python_bert_squad/bert-squad-migraphx.py
"""


import collections
import json
import os
import pathlib
import sys
import time

try:
    import numpy as np
    import tokenizers
except ImportError:
    print(
        "Could not import one or more modules in bert. Did you run 'pip install -r requirements.txt'?"
    )
    sys.exit(1)

import proteus

# isort: split

from bert import parse_args

base_path = pathlib.Path(__file__).parent.resolve()
download_dir = base_path / "../../external/artifacts/onnx/bert"
sys.path.append(str(download_dir))
try:
    from run_onnx_squad import (
        convert_examples_to_features,
        read_squad_examples,
        write_predictions,
    )
except ImportError:
    print("Could not import run_onnx_squad. Did you run 'proteus get --migraphx'?")
    sys.exit(1)


def load(client, args):
    """
    Load a worker to handle an inference request. The load returns the endpoint
    you should use for subsequent requests

    Args:
        client (proteus.client.Client): the client object
        args (argparse.Namespace): the command line arguments

    Returns:
        str: endpoint
    """

    # The only parameter the migraphx worker requires is the model file name.
    # batch and timeout are optional.
    # It will take the file name stem and search for either a *.onnx or *.mxr extension, and if
    # it finds a *.onnx file it will compile it and save the compiled model as *.mxr for
    # future use. It will read the array dimensions and data type from the model.
    parameters = proteus.RequestParameters()
    parameters.put("model", args.model)

    parameters.put("batch", args.batch_size)

    # this call requests the server to either find a running instance of the named
    # worker type, or else create one and initialize it with the parameters.
    endpoint = client.workerLoad("migraphx", parameters)

    # wait for the worker to load and compile model
    ready = False
    while not ready:
        ready = client.modelReady(endpoint)

    return endpoint


def construct_requests(eval_examples, input_ids, input_mask, segment_ids, batch_size):
    n = len(input_ids)

    requests = []
    for idx in range(0, n):
        # Create an InferenceRequest
        request = proteus.predict_api.InferenceRequest()
        item = eval_examples[idx]  # class SquadExample

        # Depending on the model, it will require one or more input tensors. The values for name, datatype, shape will also be model-dependent.

        input_n = proteus.predict_api.InferenceRequestInput()
        input_n.name = f"input_ids:0"
        input_n.datatype = proteus.DataType.INT64
        input_n.shape = (
            1,
            256,
        )
        input_n.setInt64Data(input_ids[idx : idx + batch_size])
        request.addInputTensor(input_n)

        input_n = proteus.predict_api.InferenceRequestInput()
        input_n.name = f"input_mask:0"
        input_n.datatype = proteus.DataType.INT64
        input_n.shape = (
            1,
            256,
        )
        input_n.setInt64Data(input_mask[idx : idx + batch_size])
        request.addInputTensor(input_n)

        input_n = proteus.predict_api.InferenceRequestInput()
        input_n.name = f"segment_ids:0"
        input_n.datatype = proteus.DataType.INT64
        input_n.shape = (
            1,
            256,
        )
        input_n.setInt64Data(segment_ids[idx : idx + batch_size])
        request.addInputTensor(input_n)

        # This comes from the first argument; I think it's supposed to be output names
        #       see https://onnxruntime.ai/docs/api/python/api_summary.html#load-and-run-a-model
        # item = eval_examples[idx]   # class SquadExample
        input_n = proteus.predict_api.InferenceRequestInput()
        input_n.name = f"unique_ids_raw_output___9:0"
        input_n.datatype = proteus.DataType.INT64
        input_n.shape = (1,)
        input_n.setInt64Data(np.array([item.qas_id], dtype=np.int64))
        request.addInputTensor(input_n)
        requests.append(request)
    return requests


def get_args():
    """
    The command-line arguments are parsed in two phases. There's the common
    arguments that are initialized by parse_args that are shared by all the
    Python examples in this directory and then example-specific settings are
    initialized here

    Returns:
        argparse.Namespace: the args
    """
    args = parse_args()

    root = os.getenv("PROTEUS_ROOT")

    # assign default values if these are unset
    if not args.model:
        args.model = root + "/external/artifacts/onnx/bert/bertsquad-10.onnx"

    if not args.input:
        args.input = root + "/external/artifacts/onnx/bert/inputs_amd.json"

    if not args.vocab:
        args.vocab = root + "/external/artifacts/onnx/bert/vocab.txt"

    return args


def main(args):
    print("Running the MIGraphX example for Bert in Python")

    # connect to the server
    client = proteus.clients.HttpClient(f"http://127.0.0.1:{args.http_port}")
    print("Waiting for server...", end="")
    start_server = not client.serverLive()
    if start_server:
        server = proteus.servers.Server()
        server.startHttp(args.http_port)
        while not client.serverLive():
            time.sleep(1)
    print("OK. Connected.")

    print("Loading worker...")
    endpoint = load(client, args)

    # Use read_squad_examples method from run_onnx_squad to read the input file
    eval_examples = read_squad_examples(input_file=args.input)

    print("Constructing requests...")
    max_seq_length = 256
    doc_stride = 128
    max_query_length = 64

    tokenizer = tokenizers.BertWordPieceTokenizer(args.vocab)

    # Use convert_examples_to_features method from run_onnx_squad to get parameters from the input
    # the first 3 are lists of ndarray, extra_data is a list of Feature (a namedtuple)
    input_ids, input_mask, segment_ids, extra_data = convert_examples_to_features(
        eval_examples, tokenizer, max_seq_length, doc_stride, max_query_length
    )
    # extra_data[0] is a Feature
    requests = construct_requests(
        eval_examples, input_ids, input_mask, segment_ids, args.batch_size
    )

    print(f"Sending {len(requests)} request(s)...")
    responses = proteus.client_operators.inferAsyncOrdered(client, endpoint, requests)
    print("Client received inference reply")

    print("Postprocessing...")
    raw_result = collections.namedtuple(
        "RawResult", ["unique_id", "start_logits", "end_logits"]
    )
    all_results = []
    for it, response in enumerate(responses):
        # the migraphx worker doesn't support names for output channels,
        # so we get them by position instead.
        out0 = response.getOutputs()[0]
        assert out0.datatype == proteus.DataType.FP32

        out1 = response.getOutputs()[1]
        assert out1.datatype == proteus.DataType.FP32

        # The third output ("unique_ids:0") is a single int64 which our example doesn't use.
        # Put our results into a RawResult structure
        all_results.append(
            raw_result(
                unique_id=it,
                start_logits=out1.getFp32Data(),
                end_logits=out0.getFp32Data(),
            )
        )

    output_dir = base_path / "predictions"
    os.makedirs(output_dir, exist_ok=True)
    output_prediction_file = output_dir / "predictions.json"
    output_nbest_file = output_dir / "nbest_predictions.json"
    n_best_size = 20
    max_answer_length = 301
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

    with open(output_prediction_file, "r") as json_file:
        test_data = json.load(json_file)
        print(json.dumps(test_data, indent=2))
    print("Done! Your output file is ", output_prediction_file)


if __name__ == "__main__":
    args = get_args()
    main(args)
