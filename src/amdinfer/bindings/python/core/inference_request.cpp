// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief Implements the Python bindings for the inference_request.hpp header
 */

#include "amdinfer/core/inference_request.hpp"

#include <pybind11/attr.h>      // for keep_alive
#include <pybind11/cast.h>      // for arg
#include <pybind11/numpy.h>     // for array_t
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include <array>          // for array
#include <cstring>        // for memcpy
#include <sstream>        // IWYU pragma: keep
#include <unordered_map>  // for unordered_map

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/bindings/python/helpers/keep_alive.hpp"  // for keep_alive
#include "amdinfer/bindings/python/helpers/print.hpp"       // for toString
#include "amdinfer/core/inference_response.hpp"

namespace py = pybind11;

namespace amdinfer {

template <typename T>
py::array_t<T> getData(const amdinfer::InferenceRequestInput &self) {
  auto *data = static_cast<T *>(self.getData());
  return py::array_t<T>(self.getSize(), data);

  // return py::memoryview::from_memory(data->data(), self.getSize());
}

template <typename T>
void setData(amdinfer::InferenceRequestInput &self, py::array_t<T> &b) {
  // NOLINTNEXTLINE(google-readability-casting)
  self.setData((void *)(b.data()));
}

void wrapInferenceRequestInput(py::module_ &m) {
  py::class_<InferenceRequestInput, InferenceTensor>(m, "InferenceRequestInput")
    .def(py::init<>(), DOCS(InferenceRequestInput, InferenceRequestInput))
    .def(py::init<const Tensor &>(),
         DOCS(InferenceRequestInput, InferenceRequestInput, 2),
         py::arg("tensor"))
    .def(py::init<void *, std::vector<int64_t>, DataType, std::string>(),
         DOCS(InferenceRequestInput, InferenceRequestInput, 3), py::arg("data"),
         py::arg("shape"), py::arg("data_type"), py::arg("data") = "")
    .def("setUint8Data", &setData<uint8_t>, KeepAliveAssign())
    .def("setUint16Data", &setData<uint16_t>, KeepAliveAssign())
    .def("setUint32Data", &setData<uint32_t>, KeepAliveAssign())
    .def("setUint64Data", &setData<uint64_t>, KeepAliveAssign())
    .def("setInt8Data", &setData<int8_t>, KeepAliveAssign())
    .def("setInt16Data", &setData<int16_t>, KeepAliveAssign())
    .def("setInt32Data", &setData<int32_t>, KeepAliveAssign())
    .def("setInt64Data", &setData<int64_t>, KeepAliveAssign())
    .def("setFp16Data", &setData<amdinfer::fp16>, KeepAliveAssign())
    .def("setFp32Data", &setData<float>, KeepAliveAssign())
    .def("setFp64Data", &setData<double>, KeepAliveAssign())
    .def("setStringData", &setData<unsigned char>, KeepAliveAssign())
    .def("getUint8Data", &getData<uint8_t>, KeepAliveReturn())
    .def("getUint16Data", &getData<uint16_t>, KeepAliveReturn())
    .def("getUint32Data", &getData<uint32_t>, KeepAliveReturn())
    .def("getUint64Data", &getData<uint64_t>, KeepAliveReturn())
    .def("getInt8Data", &getData<int8_t>, KeepAliveReturn())
    .def("getInt16Data", &getData<int16_t>, KeepAliveReturn())
    .def("getInt32Data", &getData<int32_t>, KeepAliveReturn())
    .def("getInt64Data", &getData<int64_t>, KeepAliveReturn())
    .def("getFp16Data", &getData<amdinfer::fp16>, KeepAliveReturn())
    .def("getFp32Data", &getData<float>, KeepAliveReturn())
    .def("getFp64Data", &getData<double>, KeepAliveReturn())
    .def("getStringData", &getData<char>, KeepAliveReturn())
    .def("__repr__",
         [](const InferenceRequestInput &self) {
           return "InferenceRequestInput(" + std::to_string(self.getSize()) +
                  ")";
         })
    .def("__str__", &amdinfer::toString<InferenceRequestInput>);
}

void wrapInferenceRequestOutput(py::module_ &m) {
  py::class_<InferenceRequestOutput>(m, "InferenceRequestOutput")
    .def(py::init<>(), DOCS(InferenceRequestOutput))
    .def_property("name", &InferenceRequestOutput::getName,
                  &InferenceRequestOutput::setName)
    .def_property("data", &InferenceRequestOutput::getData,
                  &InferenceRequestOutput::setData)
    .def_property("parameters", &InferenceRequestOutput::getParameters,
                  &InferenceRequestOutput::setParameters)
    .def("__repr__", [](const InferenceRequestOutput &self) {
      (void)self;
      return "InferenceRequestOutput\n";
    });
}

void wrapInferenceRequest(py::module_ &m) {
  // need to use a function pointer to disambiguate overloaded function
  // NOLINTNEXTLINE(readability-identifier-naming)
  auto addInputTensor =
    static_cast<void (InferenceRequest::*)(InferenceRequestInput)>(
      &InferenceRequest::addInputTensor);
  py::class_<InferenceRequest>(m, "InferenceRequest")
    .def(py::init<>(), DOCS(InferenceRequest, InferenceRequest))
    .def("propagate", &InferenceRequest::propagate,
         DOCS(InferenceRequest, propagate))
    .def_property("id", &InferenceRequest::getID, &InferenceRequest::setID)
    .def_property("parameters", &InferenceRequest::getParameters,
                  &InferenceRequest::setParameters)
    .def("getOutputs", &InferenceRequest::getOutputs,
         py::return_value_policy::reference_internal,
         DOCS(InferenceRequest, getOutputs))
    .def("getInputs", &InferenceRequest::getInputs,
         py::return_value_policy::reference_internal,
         DOCS(InferenceRequest, getInputs))
    .def("getInputSize", &InferenceRequest::getInputSize,
         DOCS(InferenceRequest, getInputSize))
    .def("addInputTensor", addInputTensor, py::arg("input"), KeepAliveAssign(),
         DOCS(InferenceRequest, addInputTensor))
    .def("addOutputTensor", &InferenceRequest::addOutputTensor,
         py::arg("output"), KeepAliveAssign(),
         DOCS(InferenceRequest, addOutputTensor))
    // .def("setCallback", [](InferenceRequest& self, amdinfer::Callback
    // callback) {
    //   self.setCallback(std::move(callback));
    // })
    // .def("runCallback", &InferenceRequest::runCallback, py::arg("response"))
    // .def("runCallbackOnce", &InferenceRequest::runCallbackOnce,
    //      py::arg("response"))
    // .def("runCallbackError", &InferenceRequest::runCallbackError,
    //      py::arg("error_msg"))
    .def("__repr__", [](const InferenceRequest &self) {
      (void)self;
      return "InferenceRequest";
    });
}

void wrapInferenceRequests(py::module_ &m) {
  wrapInferenceRequestInput(m);
  wrapInferenceRequestOutput(m);
  wrapInferenceRequest(m);
}

}  // namespace amdinfer
