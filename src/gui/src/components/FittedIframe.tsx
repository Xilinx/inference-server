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

import React from 'react';
import IframeResizer from 'iframe-resizer-react'
import {Loading} from 'react-admin'

interface IProps {
  src: string
}

interface IState{
  isLoading: boolean
  found: boolean
}


const fetchData = async (src: string) => {
  let url = window.location.host
  let res = await fetch("http://" + url + "/" + src, {
    method: 'HEAD',
  })
  return res;
}

class FittedIframe extends React.Component<IProps, IState> {
  state: IState;

  constructor(props: IProps) {
    super(props)
    this.state = { isLoading: true, found: false}
  }

  componentDidMount(){
    fetchData(this.props.src).then(resp => {
      if (resp.ok){
        this.setState({found: true});
      }
      this.setState({isLoading: false});
    })
  }

  render() {
    if(this.state.isLoading){
      return <Loading loadingPrimary="Please wait" loadingSecondary="loading..." />
    } else {
      if(this.state.found){
        return (
          <IframeResizer
            title="static_html"
            src={this.props.src}
            style={{ minHeight: '100%', minWidth: '100%' }}
            scrolling
            frameBorder="0"
          />
        );
      } else {
        return (
          <h1>
            Requested resource not found
          </h1>
        );
      }
    }
  }
}

export default FittedIframe;
