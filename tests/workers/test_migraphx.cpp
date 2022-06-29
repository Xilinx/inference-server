// Copyright 2022 AMD Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// #include <iostream>
// #include <numeric>
// #include <migraphx.hpp>              // MIGraphX C++ API

/** Runs an MiGraphX inference straight from the API, without going
 * through the Inference Server.  Use to verify that results for the 
 * same model are the same, or to do performance checks on the GPU that bypass
 * the time overhead of the REST interface.
 * 
 * 
 * */

// int main(int argc, char* argv[]){
//     std::string model_filename = "/workspace/proteus/external/artifacts/migraphx/resnet50-v1-7/resnet50-v1-7.onnx";
//     if(argc > 0)
//         model_filename = std::string(argv[0]);

//     migraphx::api::program program = migraphx::parse_onnx(model_filename.c_str());
//     migraphx::compile_options options;
//     options.set_offload_copy();
//     program.compile(migraphx::target("gpu"), options);

//     std::string image_filename = "/workspace/proteus/external/artifacts/migraphx/yflower.jpg";
//     if(argc > 1)
//         image_filename = std::string(argv[1]);

//     migraphx::program_parameters pp;
//     // the shape taken from the Resnet50 model should be 224x224x3.  Make the input image the same size.
//     auto param_shapes = program.get_parameter_shapes();
//     for(auto&& name : param_shapes.names())
//     {
//         // pp.add(name, migraphx::argument::generate(param_shapes[name]));
//     }

//     // run the inference
//     auto outputs = program.eval(pp);
//     auto output  = outputs[0];
//     auto lens    = output.get_shape().lengths();
//     auto elem_num =
//         std::accumulate(lens.begin(), lens.end(), 1, std::multiplies<std::size_t>());
//     float* data_ptr = reinterpret_cast<float*>(output.data());
//     std::vector<float> ret(data_ptr, data_ptr + elem_num);
// }