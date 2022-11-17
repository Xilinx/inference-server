// Copyright 2021 Xilinx, Inc.
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

import * as React from 'react';
import { Route } from 'react-router-dom';
import About from './pages/about';
import Coverage from './pages/coverage';
import {Doxygen, Sphinx} from './pages/docs';
import Demos from "./pages/demos"
import StreamingDemo from "./pages/streaming_demo"

const customRoutes = [
    <Route exact path="/about" render={() => <About />} />,
    <Route exact path="/coverage" render={() => <Coverage />} />,
    <Route exact path="/demos" render={() => <Demos />} />,
    <Route exact path="/docsSphinx" render={() => <Sphinx />} />,
    <Route exact path="/docsDoxygen" render={() => <Doxygen />} />,
    <Route exact path="/streaming_demo" render={() => <StreamingDemo />} />,
];

export default customRoutes;
