// Copyright 2022 Xilinx, Inc.
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
 * @brief Implements the Python bindings for the predict_api.hpp header
 */

#include "amdinfer/core/predict_api.hpp"

#include <pybind11/attr.h>      // for keep_alive
#include <pybind11/cast.h>      // for arg
#include <pybind11/numpy.h>     // for array_t
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include <array>          // for array
#include <sstream>        // IWYU pragma: keep
#include <unordered_map>  // for unordered_map

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/bindings/python/helpers/print.hpp"       // for toString

namespace py = pybind11;

namespace amdinfer {

// https://pybind11.readthedocs.io/en/stable/advanced/functions.html#keep-alive
constexpr auto kKeepAliveReturn = 0;
constexpr auto kKeepAliveSelf = 1;
constexpr auto kKeepAliveArg0 = 2;

using keep_alive_return = py::keep_alive<kKeepAliveReturn, kKeepAliveSelf>;
using keep_alive_assign = py::keep_alive<kKeepAliveSelf, kKeepAliveArg0>;

void wrapRequestParameters(py::module_ &m) {
  using amdinfer::RequestParameters;

  py::class_<RequestParameters, std::shared_ptr<RequestParameters>>(
    m, "RequestParameters")
    .def(py::init<>(), DOCS(RequestParameters))
    .def("put",
         py::overload_cast<const std::string &, bool>(&RequestParameters::put),
         DOCS(RequestParameters, put))
    .def(
      "put",
      py::overload_cast<const std::string &, double>(&RequestParameters::put),
      DOCS(RequestParameters, put, 2))
    .def(
      "put",
      py::overload_cast<const std::string &, int32_t>(&RequestParameters::put),
      DOCS(RequestParameters, put, 3))
    .def("put",
         py::overload_cast<const std::string &, const std::string &>(
           &RequestParameters::put),
         DOCS(RequestParameters, put, 4))
    .def("put",
         py::overload_cast<const std::string &, const char *>(
           &RequestParameters::put),
         DOCS(RequestParameters, put, 5))
    .def("getBool", &RequestParameters::get<bool>, DOCS(RequestParameters, get))
    .def("getFloat", &RequestParameters::get<double>,
         DOCS(RequestParameters, get))
    .def("getInt", &RequestParameters::get<int32_t>,
         DOCS(RequestParameters, get))
    .def("getString", &RequestParameters::get<std::string>,
         DOCS(RequestParameters, get))
    .def("has", &RequestParameters::has, DOCS(RequestParameters, has),
         py::arg("key"))
    .def("erase", &RequestParameters::erase, DOCS(RequestParameters, erase))
    .def("empty", &RequestParameters::empty, DOCS(RequestParameters, empty))
    .def("size", &RequestParameters::size, DOCS(RequestParameters, size))
    .def("__len__", &RequestParameters::size)
    .def("__bool__",
         [](const RequestParameters &self) { return !self.empty(); })
    .def(
      "__iter__",
      [](const RequestParameters &self) {
        return py::make_iterator(self.cbegin(), self.cend());
      },
      keep_alive_return())
    .def("__repr__",
         [](const RequestParameters &self) {
           return "RequestParameters(" + std::to_string(self.size()) + ")\n";
         })
    .def("__str__", &amdinfer::toString<RequestParameters>);
}

// refer to cppreference for std::visit
// helper type for the visitor #4
// template <class... Ts>
// struct overloaded : Ts... {
//   using Ts::operator()...;
// };
// // explicit deduction guide (not needed as of C++20)
// template <class... Ts>
// overloaded(Ts...) -> overloaded<Ts...>;

//? Trying to auto-convert RequestParameters <-> dict but it's not working
// namespace pybind11 { namespace detail {
//     template <> struct type_caster<amdinfer::RequestParameters> {
//     public:
//         PYBIND11_TYPE_CASTER(amdinfer::RequestParameters,
//         const_name("RequestParameters"));

//         // Conversion part 1 (Python->C++)
//         bool load(handle src, bool) {
//           /* Try converting into a Python dictionary */
//           auto tmp = py::cast<py::dict>(src);
//           if (!tmp)
//             return false;

//           for(const auto& [key, param] : tmp){
//             const auto& key_str = PyUnicode_AsUTF8(key.ptr());
//             bool foo;
//             try{
//               PyArg_ParseTuple(param.ptr(), "p", &foo);
//             } catch(...){
//               int foo2;
//               try{

//                 PyArg_ParseTuple(param.ptr(), "i", &foo);
//               } catch(...){
//                 double foo3;
//                 try{
//                   PyArg_ParseTuple(param.ptr(), "d", &foo);
//                 } catch(...){
//                   const char* foo4;
//                   PyArg_ParseTuple(param.ptr(), "s", &foo);
//                   value.put(key_str, foo4);
//                   continue;
//                 }
//                 value.put(key_str, foo3);
//                 continue;
//               }
//               value.put(key_str, foo2);
//               continue;
//             }
//             value.put(key_str, foo);
//             continue;
//           }

//           Py_DECREF(tmp.ptr());
//           // Ensure return code was OK
//           return !PyErr_Occurred();
//         }

//         /**
//          * Conversion part 2 (C++ -> Python). Ignoring policy and handle per
//          * pybind11 suggestion
//          */
//         static handle cast(amdinfer::RequestParameters src,
//         return_value_policy, handle) {
//           py::dict tmp;
//           for (const auto& pair : src) {
//             auto& key = pair.first;
//             const auto& param = pair.second;
//             std::visit(
//               overloaded{[&](bool arg) { tmp[pybind11::cast(key)] = arg; },
//                         [&](double arg) { tmp[pybind11::cast(key)] = arg; },
//                         [&](int32_t arg) { tmp[pybind11::cast(key)] = arg; },
//                         [&](const std::string& arg) {
//                         tmp[pybind11::cast(key)] = arg; }},
//               param);
//           }
//           return tmp;
//         }
//     };
// }} // namespace pybind11::detail

void wrapServerMetadata(py::module_ &m) {
  py::class_<ServerMetadata>(m, "ServerMetadata")
    .def(py::init<>(), DOCS(ServerMetadata))
    .def_readwrite("name", &ServerMetadata::name, DOCS(ServerMetadata, name))
    .def_readwrite("version", &ServerMetadata::version,
                   DOCS(ServerMetadata, version))
    .def_readwrite("extensions", &ServerMetadata::extensions,
                   DOCS(ServerMetadata, extensions));
}

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
  // need to use function pointer to disambiguate overloaded function
  // NOLINTNEXTLINE(readability-identifier-naming)
  auto setShape =
    static_cast<void (InferenceRequestInput::*)(const std::vector<uint64_t> &)>(
      &InferenceRequestInput::setShape);

  py::class_<InferenceRequestInput>(m, "InferenceRequestInput")
    .def(py::init<>(), DOCS(InferenceRequestInput))
    .def(py::init<void *, std::vector<uint64_t>, amdinfer::DataType,
                  std::string>(),
         DOCS(InferenceRequestInput, 2), py::arg("data"), py::arg("shape"),
         py::arg("dataType"), py::arg("name") = "")
    .def("setUint8Data", &setData<uint8_t>, keep_alive_assign())
    .def("setUint16Data", &setData<uint16_t>, keep_alive_assign())
    .def("setUint32Data", &setData<uint32_t>, keep_alive_assign())
    .def("setUint64Data", &setData<uint64_t>, keep_alive_assign())
    .def("setInt8Data", &setData<int8_t>, keep_alive_assign())
    .def("setInt16Data", &setData<int16_t>, keep_alive_assign())
    .def("setInt32Data", &setData<int32_t>, keep_alive_assign())
    .def("setInt64Data", &setData<int64_t>, keep_alive_assign())
    .def("setFp16Data", &setData<amdinfer::fp16>, keep_alive_assign())
    .def("setFp32Data", &setData<float>, keep_alive_assign())
    .def("setFp64Data", &setData<double>, keep_alive_assign())
    .def(
      "setStringData",
      [](amdinfer::InferenceRequestInput &self, std::string &str) {
        std::vector<std::byte> data;
        data.resize(str.length());
        memcpy(data.data(), str.data(), str.length());
        self.setData(std::move(data));
      },
      keep_alive_assign())
    .def("getUint8Data", &getData<uint8_t>, keep_alive_return())
    .def("getUint16Data", &getData<uint16_t>, keep_alive_return())
    .def("getUint32Data", &getData<uint32_t>, keep_alive_return())
    .def("getUint64Data", &getData<uint64_t>, keep_alive_return())
    .def("getInt8Data", &getData<int8_t>, keep_alive_return())
    .def("getInt16Data", &getData<int16_t>, keep_alive_return())
    .def("getInt32Data", &getData<int32_t>, keep_alive_return())
    .def("getInt64Data", &getData<int64_t>, keep_alive_return())
    .def("getFp16Data", &getData<amdinfer::fp16>, keep_alive_return())
    .def("getFp32Data", &getData<float>, keep_alive_return())
    .def("getFp64Data", &getData<double>, keep_alive_return())
    .def("getStringData", &getData<char>, keep_alive_return())
    .def_property("name", &InferenceRequestInput::getName,
                  &InferenceRequestInput::setName)
    .def_property("shape", &InferenceRequestInput::getShape, setShape)
    .def_property("datatype", &InferenceRequestInput::getDatatype,
                  &InferenceRequestInput::setDatatype)
    .def_property("parameters", &InferenceRequestInput::getParameters,
                  &InferenceRequestInput::setParameters)
    .def("getSize", &InferenceRequestInput::getSize,
         DOCS(InferenceRequestInput, getSize))
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
         keep_alive_assign(), DOCS(InferenceResponse, addOutput))
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

void wrapInferenceRequest(py::module_ &m) {
  // need to use a function pointer to disambiguate overloaded function
  // NOLINTNEXTLINE(readability-identifier-naming)
  auto addInputTensor =
    static_cast<void (InferenceRequest::*)(InferenceRequestInput)>(
      &InferenceRequest::addInputTensor);
  py::class_<InferenceRequest>(m, "InferenceRequest")
    .def(py::init<>(), DOCS(InferenceRequest, InferenceRequest))
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
    .def("addInputTensor", addInputTensor, py::arg("input"),
         keep_alive_assign(), DOCS(InferenceRequest, addInputTensor))
    .def("addOutputTensor", &InferenceRequest::addOutputTensor,
         py::arg("output"), keep_alive_assign(),
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

void wrapModelMetadata(py::module_ &m) {
  py::class_<ModelMetadataTensor>(m, "ModelMetadataTensor")
    .def(py::init<const std::string &, amdinfer::DataType,
                  std::vector<uint64_t>>(),
         DOCS(ModelMetadataTensor, ModelMetadataTensor))
    .def("getName", &ModelMetadataTensor::getName,
         DOCS(ModelMetadataTensor, getName))
    .def("getDataType", &ModelMetadataTensor::getDataType,
         DOCS(ModelMetadataTensor, getDataType))
    .def("getShape", &ModelMetadataTensor::getShape,
         DOCS(ModelMetadataTensor, getShape));

  auto addInputTensor = static_cast<void (ModelMetadata::*)(
    const std::string &, amdinfer::DataType, std::vector<int>)>(
    &ModelMetadata::addInputTensor);
  auto addOutputTensor = static_cast<void (ModelMetadata::*)(
    const std::string &, amdinfer::DataType, std::vector<int>)>(
    &ModelMetadata::addOutputTensor);
  py::class_<ModelMetadata>(m, "ModelMetadata")
    .def(py::init<const std::string &, const std::string &>(),
         DOCS(ModelMetadata, ModelMetadata))
    .def("addInputTensor", addInputTensor, keep_alive_assign(),
         DOCS(ModelMetadata, addInputTensor))
    .def("addOutputTensor", addOutputTensor, py::arg("name"),
         py::arg("datatype"), py::arg("shape"), keep_alive_assign(),
         DOCS(ModelMetadata, addOutputTensor))
    .def_property("name", &ModelMetadata::getName, &ModelMetadata::setName)
    .def("getPlatform", &ModelMetadata::getPlatform,
         DOCS(ModelMetadata, getPlatform))
    .def("setReady", &ModelMetadata::setReady, DOCS(ModelMetadata, setReady))
    .def("isReady", &ModelMetadata::isReady, DOCS(ModelMetadata, isReady));
}

void wrapPredictApi(py::module_ &m) {
  wrapServerMetadata(m);
  wrapInferenceRequestInput(m);
  wrapInferenceRequestOutput(m);
  wrapInferenceResponse(m);
  wrapInferenceRequest(m);
  wrapModelMetadata(m);
}

}  // namespace amdinfer
