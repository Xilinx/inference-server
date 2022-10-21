..
    Copyright 2022 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

ResNet50
--------

These examples demonstrate how to make an inference using the ResNet50 model with the different backends supported by the inference server.

Introduction
^^^^^^^^^^^^

ResNet50 is a classification model that accepts a single input tensor that contains the input image.
For this image, it returns the probabilities that the image can be classified as any of its output classes.
The number of output classes depends on the dataset that the network is trained on.
In these examples, the default model is trained on the ImageNet dataset and has 1000 output classes.

Files
^^^^^

The files in this set of examples are:

::

    examples/resnet50
    ├── CMakeLists.txt
    ├── imagenet_classes.txt
    ├── migraphx.cpp
    ├── migraphx.py
    ├── ptzendnn.cpp
    ├── ptzendnn.py
    ├── README.rst
    ├── resnet50.hpp
    ├── resnet.py
    ├── tfzendnn.cpp
    ├── tfzendnn.py
    ├── vitis.cpp
    └── vitis.py
