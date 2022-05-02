// Copyright 2022 Xilinx Inc.
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

#include "proteus/core/predict_api.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

#include "docstrings.hpp"

namespace py = pybind11;

void wrapRequestParameters(py::module_ &m) {
  using proteus::RequestParameters;

  py::class_<RequestParameters>(m, "RequestParameters")
    .def(py::init<>(), DOC(proteus, RequestParameters))
    .def("put",
         py::overload_cast<const std::string &, bool>(&RequestParameters::put),
         DOC(proteus, RequestParameters, put))
    .def(
      "put",
      py::overload_cast<const std::string &, double>(&RequestParameters::put),
      DOC(proteus, RequestParameters, put, 2))
    .def(
      "put",
      py::overload_cast<const std::string &, int32_t>(&RequestParameters::put),
      DOC(proteus, RequestParameters, put, 3))
    .def("put",
         py::overload_cast<const std::string &, const std::string &>(
           &RequestParameters::put),
         DOC(proteus, RequestParameters, put, 4))
    .def("put",
         py::overload_cast<const std::string &, const char *>(
           &RequestParameters::put),
         DOC(proteus, RequestParameters, put, 5))
    .def("getBool", &RequestParameters::get<bool>,
         DOC(proteus, RequestParameters, get))
    .def("getFloat", &RequestParameters::get<double>,
         DOC(proteus, RequestParameters, get))
    .def("getInt", &RequestParameters::get<int32_t>,
         DOC(proteus, RequestParameters, get))
    .def("getString", &RequestParameters::get<std::string>,
         DOC(proteus, RequestParameters, get))
    .def("has", &RequestParameters::has, DOC(proteus, RequestParameters, has),
         py::arg("key"))
    .def("erase", &RequestParameters::erase,
         DOC(proteus, RequestParameters, erase))
    .def("empty", &RequestParameters::empty,
         DOC(proteus, RequestParameters, empty))
    .def("size", &RequestParameters::size,
         DOC(proteus, RequestParameters, size))
    .def(
      "__iter__",
      [](const RequestParameters &self) {
        return py::make_iterator(self.cbegin(), self.cend());
      },
      py::keep_alive<0, 1>())
    .def("__repr__",
         [](const RequestParameters &self) {
           return "RequestParameters(" + std::to_string(self.size()) + ")\n";
         })
    .def("__str__", [](const RequestParameters &self) {
      std::ostringstream os;
      os << self;
      return os.str();
    });
}

// refer to cppreference for std::visit
// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

//? Trying to auto-convert RequestParameters <-> dict but it's not working
// namespace pybind11 { namespace detail {
//     template <> struct type_caster<proteus::RequestParameters> {
//     public:
//         PYBIND11_TYPE_CASTER(proteus::RequestParameters,
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
//         static handle cast(proteus::RequestParameters src,
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

void wrapPredictApi(py::module_ &m) {
  using proteus::InferenceRequest;
  using proteus::InferenceRequestInput;
  using proteus::InferenceRequestOutput;
  using proteus::InferenceResponse;
  using proteus::ServerMetadata;

  (void)m;

  // py::module_::import("proteus").attr("RequestParameters");

  py::class_<ServerMetadata>(m, "ServerMetadata")
    .def(py::init<>(), DOC(proteus, ServerMetadata))
    .def_readwrite("name", &ServerMetadata::name,
                   DOC(proteus, ServerMetadata, name))
    .def_readwrite("version", &ServerMetadata::version,
                   DOC(proteus, ServerMetadata, version))
    .def_readwrite("extensions", &ServerMetadata::extensions,
                   DOC(proteus, ServerMetadata, extensions));

  // py::module_::import("proteus").attr("DataType");
  auto setShape =
    static_cast<void (InferenceRequestInput::*)(const std::vector<uint64_t> &)>(
      &InferenceRequestInput::setShape);

  py::class_<InferenceRequestInput>(m, "InferenceRequestInput")
    .def(py::init<>(), DOC(proteus, InferenceRequestInput))
    .def(py::init<void *, std::vector<uint64_t>, proteus::types::DataType,
                  std::string>(),
         DOC(proteus, InferenceRequestInput, 2), py::arg("data"),
         py::arg("shape"), py::arg("dataType"), py::arg("name") = "")
    .def(
      "setData",
      [](InferenceRequestInput &self, py::list b) { self.setData(b.ptr()); },
      py::keep_alive<1, 2>(), DOC(proteus, InferenceRequestInput, setData))
    // .def("setData",
    //      py::overload_cast<std::shared_ptr<std::byte>>(&InferenceRequestInput::setData),
    //      DOC(proteus, InferenceRequestInput, setData, 2))
    .def("getData", &InferenceRequestInput::getData)
    .def_property("name", &InferenceRequestInput::getName,
                  &InferenceRequestInput::setName)
    .def_property("shape", &InferenceRequestInput::getShape, setShape)
    .def_property("datatype", &InferenceRequestInput::getDatatype,
                  &InferenceRequestInput::setDatatype)
    .def_property("parameters", &InferenceRequestInput::getParameters,
                  &InferenceRequestInput::setParameters)
    .def("getSize", &InferenceRequestInput::getSize)
    .def("__repr__",
         [](const InferenceRequestInput &self) {
           return "InferenceRequestInput(" + std::to_string(self.getSize()) +
                  ")";
         })
    .def("__str__", [](const proteus::InferenceRequestInput &self) {
      std::ostringstream os;
      os << self;
      return os.str();
    });

  py::class_<InferenceRequestOutput>(m, "InferenceRequestOutput")
    .def(py::init<>(), DOC(proteus, InferenceRequestOutput))
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
  // .def("__str__", [](const InferenceRequestOutput& self) {
  //   std::ostringstream os;
  //   os << self;
  //   return os.str();
  // });

  py::class_<InferenceResponse>(m, "InferenceResponse")
    .def(py::init<>(), DOC(proteus, InferenceResponse, InferenceResponse))
    .def(py::init<const std::string &>(),
         DOC(proteus, InferenceResponse, InferenceResponse, 2))
    .def_property("id", &InferenceResponse::getID, &InferenceResponse::setID)
    .def_property("model", &InferenceResponse::getModel,
                  &InferenceResponse::setModel)
#ifdef PROTEUS_ENABLE_TRACING
    .def("getContext", &InferenceResponse::getContext)
    .def("setContext",
         [](InferenceResponse &self, proteus::StringMap context) {
           self.setContext(std::move(context));
         })
#endif
    .def("getParameters", &InferenceResponse::getParameters)
    .def("getOutputs", &InferenceResponse::getOutputs)
    .def("addOutput", &InferenceResponse::addOutput, py::arg("output"))
    .def("isError", &InferenceResponse::isError)
    .def("getError", &InferenceResponse::getError)
    .def("__repr__",
         [](const InferenceResponse &self) {
           (void)self;
           return "InferenceResponse\n";
         })
    .def("__str__", [](const InferenceResponse &self) {
      std::ostringstream os;
      os << self;
      return os.str();
    });

  auto addInputTensor =
    static_cast<void (InferenceRequest::*)(InferenceRequestInput)>(
      &InferenceRequest::addInputTensor);
  py::class_<InferenceRequest>(m, "InferenceRequest")
    .def(py::init<>(), DOC(proteus, InferenceRequest, InferenceRequest))
    .def_property("id", &InferenceRequest::getID, &InferenceRequest::setID)
    .def_property("parameters", &InferenceRequest::getParameters,
                  &InferenceRequest::setParameters)
    .def("getOutputs", &InferenceRequest::getOutputs)
    .def("getInputs", &InferenceRequest::getInputs)
    .def("getInputSize", &InferenceRequest::getInputSize)
    .def("addInputTensor", addInputTensor, py::arg("input"))
    .def("addOutputTensor", &InferenceRequest::addOutputTensor,
         py::arg("output"))
    // .def("setCallback", [](InferenceRequest& self, proteus::Callback
    // callback) {
    //   self.setCallback(std::move(callback));
    // })
    .def("runCallback", &InferenceRequest::runCallback, py::arg("response"))
    .def("runCallbackOnce", &InferenceRequest::runCallbackOnce,
         py::arg("response"))
    .def("runCallbackError", &InferenceRequest::runCallbackError,
         py::arg("error_msg"))
    .def("__repr__", [](const InferenceRequest &self) {
      (void)self;
      return "InferenceRequest\n";
    });
  // .def("__str__", [](const InferenceRequest& self) {
  //   std::ostringstream os;
  //   os << self;
  //   return os.str();
  // });

  using proteus::ModelMetadataTensor;
  py::class_<ModelMetadataTensor>(m, "ModelMetadataTensor")
    .def(py::init<const std::string &, proteus::types::DataType,
                  std::vector<uint64_t>>(),
         DOC(proteus, ModelMetadataTensor, ModelMetadataTensor))
    .def("getName", &ModelMetadataTensor::getName,
         DOC(proteus, ModelMetadataTensor, getName))
    .def("getDataType", &ModelMetadataTensor::getDataType,
         DOC(proteus, ModelMetadataTensor, getDataType))
    .def("getShape", &ModelMetadataTensor::getShape,
         DOC(proteus, ModelMetadataTensor, getShape));

  using proteus::ModelMetadata;
  auto addInputTensor2 = static_cast<void (ModelMetadata::*)(
    const std::string &, proteus::types::DataType, std::vector<int>)>(
    &ModelMetadata::addInputTensor);
  auto addOutputTensor2 = static_cast<void (ModelMetadata::*)(
    const std::string &, proteus::types::DataType, std::vector<int>)>(
    &ModelMetadata::addOutputTensor);
  py::class_<ModelMetadata>(m, "ModelMetadata")
    .def(py::init<const std::string &, const std::string &>(),
         DOC(proteus, ModelMetadata, ModelMetadata))
    .def("addInputTensor", addInputTensor2)
    .def("addOutputTensor", addOutputTensor2, py::arg("name"),
         py::arg("datatype"), py::arg("shape"))
    .def_property("name", &ModelMetadata::getName, &ModelMetadata::setName)
    .def("getPlatform", &ModelMetadata::getPlatform)
    .def("setReady", &ModelMetadata::setReady)
    .def("isReady", &ModelMetadata::isReady);
}
