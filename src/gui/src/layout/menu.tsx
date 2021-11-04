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

import * as React from 'react';
import { FC, useState } from 'react';
import { useSelector } from 'react-redux';
import InfoIcon from '@material-ui/icons/Info';
import SupervisorAccountIcon from '@material-ui/icons/SupervisorAccount';
import MapIcon from '@material-ui/icons/Map';
import LaunchIcon from '@material-ui/icons/Launch';
import AppsIcon from '@material-ui/icons/Apps';
import WorkIcon from '@material-ui/icons/Work';
import VideoLibraryIcon from '@material-ui/icons/VideoLibrary';
import { useMediaQuery, Theme, Box } from '@material-ui/core';
import {
  // useTranslate,
  DashboardMenuItem,
  MenuItemLink,
  MenuProps,
} from 'react-admin';

import SubMenu from './submenu';
import { AppState } from '../types';

type MenuName = 'menuAbout' | 'menuDeveloper' | 'menuDemos' | 'menuDocs';

const Menu: FC<MenuProps> = ({ onMenuClick, logout, dense = false }) => {
  const [state, setState] = useState({
    menuAbout: true,
    menuDeveloper: true,
    menuDemos: true,
    menuDocs: true,
  });
  // const translate = useTranslate();
  const isXSmall = useMediaQuery((theme: Theme) =>
    theme.breakpoints.down('xs')
  );
  const open = useSelector((state: AppState) => state.admin.ui.sidebarOpen);
  useSelector((state: AppState) => state.theme); // force rerender on theme change

  const handleToggle = (menu: MenuName) => {
    setState(state => ({ ...state, [menu]: !state[menu] }));
  };

  return (
    <Box mt={1}>
      {' '}
      <DashboardMenuItem onClick={onMenuClick} sidebarIsOpen={open} />
      <SubMenu
        handleToggle={() => handleToggle('menuDemos')}
        isOpen={state.menuDemos}
        sidebarIsOpen={open}
        name="Demos"
        icon={<AppsIcon/>}
        dense={dense}
      >
        <MenuItemLink
          to={`/demos`}
          primaryText="Basic"
          leftIcon={<WorkIcon />}
          onClick={onMenuClick}
          sidebarIsOpen={open}
          dense={dense}
        />
        <MenuItemLink
          to={`/streaming_demo`}
          primaryText="Streaming"
          leftIcon={<VideoLibraryIcon />}
          onClick={onMenuClick}
          sidebarIsOpen={open}
          dense={dense}
        />
      </SubMenu>
      <MenuItemLink
        to={`/about`}
        primaryText="About"
        leftIcon={<InfoIcon />}
        onClick={onMenuClick}
        sidebarIsOpen={open}
        dense={dense}
      />
      <SubMenu
        handleToggle={() => handleToggle('menuDocs')}
        isOpen={state.menuDocs}
        sidebarIsOpen={open}
        name="Documentation"
        icon={<LaunchIcon/>}
        dense={dense}
      >
        <MenuItemLink
          to={`/docsDoxygen`}
          primaryText="Doxygen"
          // leftIcon={<LaunchIcon />}
          onClick={onMenuClick}
          sidebarIsOpen={open}
          dense={dense}
        />
        <MenuItemLink
          to={`/docsSphinx`}
          primaryText="Sphinx"
          // leftIcon={<LaunchIcon />}
          onClick={onMenuClick}
          sidebarIsOpen={open}
          dense={dense}
        />
      </SubMenu>
      <SubMenu
        handleToggle={() => handleToggle('menuDeveloper')}
        isOpen={state.menuDeveloper}
        sidebarIsOpen={open}
        name="Developer"
        icon={<SupervisorAccountIcon/>}
        dense={dense}
      >
        <MenuItemLink
          to={`/coverage`}
          primaryText="Coverage Report"
          leftIcon={<MapIcon />}
          onClick={onMenuClick}
          sidebarIsOpen={open}
          dense={dense}
        />
      </SubMenu>
      {isXSmall && logout}
    </Box>
  );
};

export default Menu;
