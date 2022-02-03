# Copyright 2021 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)

function(enable_ipo_on_target target)

if(IPO_SUPPORTED)
    set_property(TARGET ${target}
        PROPERTY INTERPROCEDURAL_OPTIMIZATION ${PROTEUS_ENABLE_IPO}
    )
endif()

endfunction()
