/**
 * Copyright 2021 Xilinx Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as React from "react";
import { Admin, ListGuesser, Resource } from 'react-admin';
import jsonServerProvider from 'ra-data-json-server';
// import { PostList } from './pages/posts';
// import { UserList } from './pages/users';
import customRoutes from './routes';
import Layout from './layout/layout';
import Dashboard from './pages/dashboard'
import {kDefaultHttpPort} from './build_options'

const dataProvider = jsonServerProvider('http://localhost:' + kDefaultHttpPort);
const App = () => (
  <Admin
    dataProvider={dataProvider}
    layout={Layout}
    dashboard={Dashboard}
    customRoutes={customRoutes}
    disableTelemetry
  >
    {/* This is a pointless Resource but we need to add something here... */}
    <Resource name="v2" options={{ label: 'About' }} list={ListGuesser} />
  </Admin>
);

export default App;
