
# This example contains the minimal Python commands necessary to bring up
# the migraphx worker and verify that it runs.  The model file in external/artifacts is 
# not in the git repo. and must be fetched with git or with 
# #  proteus get

# scp -r .\resnet50-v2-7\ bpickrel@192.169.1.115:is-fork/inference-server/temp/


import proteus
server = proteus.Server()
client = proteus.RestClient("127.0.0.1:8998")
client.wait_until_live()

# +load worker:
parameters = {"model": "/workspace/proteus/external/artifacts/migraphx/resnet50v2/resnet50-v2-7.onnx",
              "input": "/workspace/proteus/external/artifacts/migraphx/resnet50v2test_data_set_0/input_0.pb"
			  }

response = client.load("Migraphx", parameters)




# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.onnx
# https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet50-v2-7.tar.gz
