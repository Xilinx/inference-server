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
 * @brief Implements the Python bindings for pre_post
 */

#include "amdinfer/pre_post/image_preprocess.hpp"  // for ImageOrder

#include <pybind11/cast.h>      // for arg, cast
#include <pybind11/numpy.h>     // for array_t
#include <pybind11/pybind11.h>  // for sequence, module_
#include <pybind11/stl.h>       // IWYU pragma: keep

#include <array>    // for array
#include <cstdint>  // for int8_t
#include <opencv2/core.hpp>
#include <string>  // for string
#include <vector>  // for vector

#include "amdinfer/bindings/python/core/bind_fp16.hpp"
#include "amdinfer/core/inference_request.hpp"
#include "amdinfer/core/inference_response.hpp"
#include "amdinfer/pre_post/resnet50_postprocess.hpp"  // for resnet50Postpr...

// InferenceResponseOutput needs the full definition, not a forward declare
// IWYU pragma: no_include "amdinfer/declarations.hpp"

namespace py = pybind11;

namespace amdinfer {

template <typename T>
using ImagePreprocessOptions = pre_post::ImagePreprocessOptions<T, 3>;

template <typename T>
void addPreprocessOptions(const py::module& m, const char* name) {
  py::class_<ImagePreprocessOptions<T>>(m, name)
    .def(py::init<>())
    .def_readwrite("assign", &ImagePreprocessOptions<T>::assign)
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
    .def_readwrite("resize", &ImagePreprocessOptions<T>::resize)
    .def_readwrite("std", &ImagePreprocessOptions<T>::std);
}

template <typename T>
auto imagePreprocess(const std::vector<std::string>& paths,
                     const ImagePreprocessOptions<T>& options) {
  auto images = pre_post::imagePreprocess(paths, options);
  py::array_t<T> ret = py::cast(images);
  return ret;
}

void wrapPrePost(py::module_& m) {
  m.def(
    "resnet50PostprocessInt8",
    [](const InferenceResponseOutput& output, int k) {
      return pre_post::resnet50Postprocess<int8_t>(
        static_cast<int8_t*>(output.getData()), output.getSize(), k);
    }

    ,
    py::arg("output"), py::arg("k"));
  m.def(
    "resnet50PostprocessFp16",
    [](const InferenceResponseOutput& output, int k) {
      return pre_post::resnet50Postprocess<fp16>(
        static_cast<fp16*>(output.getData()), output.getSize(), k);
    },
    py::arg("output"), py::arg("k"));
  m.def(
    "resnet50PostprocessFp32",
    [](const InferenceResponseOutput& output, int k) {
      return pre_post::resnet50Postprocess<float>(
        static_cast<float*>(output.getData()), output.getSize(), k);
    },
    py::arg("output"), py::arg("k"));
  m.def(
    "resnet50PostprocessFloat",
    [](const InferenceResponseOutput& output, int k) {
      PyErr_WarnEx(PyExc_DeprecationWarning,
                   "resnet50PostprocessFloat() is deprecated, use "
                   "resnet50PostprocessFp32() instead",
                   1);
      return pre_post::resnet50Postprocess<float>(
        static_cast<float*>(output.getData()), output.getSize(), k);
    },
    py::arg("output"), py::arg("k"));

  py::enum_<pre_post::ImageOrder>(m, "ImageOrder")
    .value("NHWC", pre_post::ImageOrder::NHWC)
    .value("NCHW", pre_post::ImageOrder::NCHW);

  addPreprocessOptions<int8_t>(m, "ImagePreprocessOptionsInt8");
  addPreprocessOptions<fp16>(m, "ImagePreprocessOptionsFp16");
  addPreprocessOptions<float>(m, "ImagePreprocessOptionsFloat");

  m.def("imagePreprocessInt8", &imagePreprocess<int8_t>, py::arg("paths"),
        py::arg("options"));
  // deprecated
  m.def("imagePreprocessFloat", &imagePreprocess<float>, py::arg("paths"),
        py::arg("options"));
  m.def("imagePreprocessFp32", &imagePreprocess<float>, py::arg("paths"),
        py::arg("options"));
  m.def("imagePreprocessFp16", &imagePreprocess<fp16>, py::arg("paths"),
        py::arg("options"));
}

}  // namespace amdinfer
