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
 * @brief Implements the Python bindings for the inference_response.hpp header
 */

#include "amdinfer/core/inference_response.hpp"

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
#include "amdinfer/core/inference_request.hpp"

namespace py = pybind11;

namespace amdinfer {

void wrapInferenceResponse(py::module_ &m) {
  py::class_<InferenceResponse>(m, "InferenceResponse")
    .def(py::init<>(), DOCS(InferenceResponse, InferenceResponse))
    .def(py::init<const std::string &>(),
         DOCS(InferenceResponse, InferenceResponse, 2))
    .def_property("id", &InferenceResponse::getID, &InferenceResponse::setID)
    .def_property("model", &InferenceResponse::getModel,
                  &InferenceResponse::setModel)
#ifdef AMDINFER_ENABLE_TRACING
    .def("getContext", &InferenceResponse::getContext)
    .def("setContext",
         [](InferenceResponse &self, amdinfer::StringMap context) {
           self.setContext(std::move(context));
         })
#endif
    .def("getParameters", &InferenceResponse::getParameters,
         DOCS(InferenceResponse, getParameters))
    .def("getOutputs", &InferenceResponse::getOutputs,
         py::return_value_policy::reference_internal,
         DOCS(InferenceResponse, getOutputs))
    .def("addOutput", &InferenceResponse::addOutput, py::arg("output"),
         KeepAliveAssign(), DOCS(InferenceResponse, addOutput))
    .def("isError", &InferenceResponse::isError,
         DOCS(InferenceResponse, isError))
    .def("getError", &InferenceResponse::getError,
         DOCS(InferenceResponse, getError))
    .def("__repr__",
         [](const InferenceResponse &self) {
           (void)self;
           return "InferenceResponse\n";
         })
    .def("__str__", &amdinfer::toString<InferenceResponse>);
}

template <typename T>
py::array_t<T> getData(const amdinfer::InferenceResponseOutput &self) {
  auto *data = static_cast<T *>(self.getData());
  return py::array_t<T>(self.getSize(), data);

  // return py::memoryview::from_memory(data->data(), self.getSize());
}

template <typename T>
void setData(amdinfer::InferenceResponseOutput &self, py::array_t<T> &b) {
  std::vector<std::byte> data;
  data.resize(b.size());
  memcpy(data.data(), b.data(), b.size());
  self.setData(std::move(data));
}

void wrapInferenceResponseOutput(py::module_ &m) {
  // need to use function pointer to disambiguate overloaded function
  // NOLINTNEXTLINE(readability-identifier-naming)
  auto setShape =
    static_cast<void (InferenceResponseOutput::*)(std::vector<int64_t>)>(
      &InferenceResponseOutput::setShape);

  py::class_<InferenceResponseOutput, InferenceTensor>(
    m, "InferenceResponseOutput")
    .def(py::init<>(), DOCS(InferenceResponseOutput))
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
    .def(
      "setStringData",
      [](amdinfer::InferenceResponseOutput &self, std::string &str) {
        std::vector<std::byte> data;
        data.resize(str.size());
        memcpy(data.data(), str.data(), str.size());
        self.setData(std::move(data));
      },
      KeepAliveAssign())
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
    .def_property("name", &InferenceResponseOutput::getName,
                  &InferenceResponseOutput::setName)
    .def_property("shape", &InferenceResponseOutput::getShape, setShape)
    .def_property("datatype", &InferenceResponseOutput::getDatatype,
                  &InferenceResponseOutput::setDatatype)
    .def_property("parameters", &InferenceResponseOutput::getParameters,
                  &InferenceResponseOutput::setParameters)
    .def("getSize", &InferenceResponseOutput::getSize, DOCS(Tensor, getSize))
    .def("__repr__",
         [](const InferenceResponseOutput &self) {
           return "InferenceResponseOutput(" + std::to_string(self.getSize()) +
                  ")";
         })
    .def("__str__", &amdinfer::toString<InferenceResponseOutput>);
}

void wrapInferenceResponses(py::module &m) {
  wrapInferenceResponseOutput(m);
  wrapInferenceResponse(m);
}

}  // namespace amdinfer
