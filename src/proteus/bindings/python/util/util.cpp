// Copyright 2022 Advanced Micro Devices, Inc.
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

/**
 * @file
 * @brief Implements the Python bindings for the server.hpp header
 */

#include <pybind11/cast.h>  // for arg
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include "proteus/bindings/python/helpers/docstrings.hpp"
#include "proteus/core/predict_api.hpp"
#include "proteus/util/pre_post/image_preprocess.hpp"
#include "proteus/util/pre_post/resnet50_postprocess.hpp"

namespace py = pybind11;

template <typename T>
using ImagePreprocessOptions = proteus::util::ImagePreprocessOptions<T, 3>;

template <typename T>
void addPreprocessOptions(const py::module& m, const char* name) {
  py::class_<ImagePreprocessOptions<T>>(m, name)
    .def(py::init<>())
    .def_readwrite("height", &ImagePreprocessOptions<T>::height)
    .def_readwrite("width", &ImagePreprocessOptions<T>::width)
    .def_readwrite("channels", &ImagePreprocessOptions<T>::channels)
    .def_readwrite("convert_color", &ImagePreprocessOptions<T>::convert_color)
    .def_readwrite("color_code", &ImagePreprocessOptions<T>::color_code)
    .def_readwrite("convert_type", &ImagePreprocessOptions<T>::convert_type)
    .def_readwrite("type", &ImagePreprocessOptions<T>::type)
    .def_readwrite("convert_scale", &ImagePreprocessOptions<T>::convert_scale)
    .def_readwrite("normalize", &ImagePreprocessOptions<T>::normalize)
    .def_readwrite("order", &ImagePreprocessOptions<T>::order)
    .def_readwrite("mean", &ImagePreprocessOptions<T>::mean)
    .def_readwrite("std", &ImagePreprocessOptions<T>::std);
}

template <typename T>
auto imagePreprocess(const std::vector<std::string>& paths,
                     const ImagePreprocessOptions<T>& options) {
  auto images = proteus::util::imagePreprocess(paths, options);
  py::array_t<T> ret = py::cast(images);
  return ret;
}

void wrapUtil(py::module_& m) {
  py::module pre_post = m.def_submodule("pre_post", "pre_post documentation");
  pre_post.def(
    "resnet50PostprocessInt8",
    [](const proteus::InferenceResponseOutput& output, int k) {
      return proteus::util::resnet50Postprocess<int8_t>(
        static_cast<int8_t*>(output.getData()), output.getSize(), k);
    }

    ,
    py::arg("output"), py::arg("k"));
  pre_post.def(
    "resnet50PostprocessFloat",
    [](const proteus::InferenceResponseOutput& output, int k) {
      return proteus::util::resnet50Postprocess<float>(
        static_cast<float*>(output.getData()), output.getSize(), k);
    },
    py::arg("output"), py::arg("k"));

  py::enum_<proteus::util::ImageOrder>(pre_post, "ImageOrder")
    .value("NHWC", proteus::util::ImageOrder::NHWC)
    .value("NCHW", proteus::util::ImageOrder::NCHW);

  addPreprocessOptions<int8_t>(pre_post, "ImagePreprocessOptionsInt8");
  addPreprocessOptions<float>(pre_post, "ImagePreprocessOptionsFloat");

  pre_post.def("imagePreprocessInt8", &imagePreprocess<int8_t>,
               py::arg("paths"), py::arg("options"));
  pre_post.def("imagePreprocessFloat", &imagePreprocess<float>,
               py::arg("paths"), py::arg("options"));
}
