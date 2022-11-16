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

export const darkTheme = {
  palette: {
    primary: {
      main: '#90caf9',
    },
    secondary: {
      main: '#FBBA72',
    },
    type: 'dark' as 'dark', // Switching the dark mode on is a single property value change.
  },
  overrides: {
    MuiAppBar: {
      colorSecondary: {
        color: '#ffffffb3',
        backgroundColor: '#616161e6',
      },
    },
    MuiButtonBase: {
      root: {
        '&:hover:active::after': {
          // recreate a static ripple color
          // use the currentColor to make it work both for outlined and contained buttons
          // but to dim the background without dimming the text,
          // put another element on top with a limited opacity
          content: '""',
          display: 'block',
          width: '100%',
          height: '100%',
          position: 'absolute',
          top: 0,
          right: 0,
          backgroundColor: 'currentColor',
          opacity: 0.3,
          borderRadius: 'inherit',
        },
      },
    },
  },
  props: {
    MuiButtonBase: {
      // disable ripple for perf reasons
      disableRipple: true,
    },
  },
};

export const lightTheme = {
  palette: {
    primary: {
      main: '#4f3cc9',
    },
    secondary: {
      light: '#5f5fc4',
      main: '#283593',
      dark: '#001064',
      contrastText: '#fff',
    },
    background: {
      default: '#fcfcfe',
    },
    type: 'light' as 'light',
  },
  shape: {
    borderRadius: 10,
  },
  overrides: {
    RaMenuItemLink: {
      root: {
        borderLeft: '3px solid #fff',
      },
      active: {
        borderLeft: '3px solid #4f3cc9',
      },
    },
    MuiPaper: {
      elevation1: {
        boxShadow: 'none',
      },
      root: {
        border: '1px solid #e0e0e3',
        backgroundClip: 'padding-box',
      },
    },
    MuiButton: {
      contained: {
        backgroundColor: '#fff',
        color: '#4f3cc9',
        boxShadow: 'none',
      },
    },
    MuiButtonBase: {
      root: {
        '&:hover:active::after': {
          // recreate a static ripple color
          // use the currentColor to make it work both for outlined and contained buttons
          // but to dim the background without dimming the text,
          // put another element on top with a limited opacity
          content: '""',
          display: 'block',
          width: '100%',
          height: '100%',
          position: 'absolute',
          top: 0,
          right: 0,
          backgroundColor: 'currentColor',
          opacity: 0.3,
          borderRadius: 'inherit',
        },
      },
    },
    MuiAppBar: {
      colorSecondary: {
        color: '#808080',
        backgroundColor: '#fff',
      },
    },
    MuiLinearProgress: {
      colorPrimary: {
        backgroundColor: '#f5f5f5',
      },
      barColorPrimary: {
        backgroundColor: '#d7d7d7',
      },
    },
    MuiFilledInput: {
      root: {
        backgroundColor: 'rgba(0, 0, 0, 0.04)',
        '&$disabled': {
          backgroundColor: 'rgba(0, 0, 0, 0.04)',
        },
      },
    },
    MuiSnackbarContent: {
      root: {
        border: 'none',
      },
    },
  },
  props: {
    MuiButtonBase: {
      // disable ripple for perf reasons
      disableRipple: true,
    },
  },
};
