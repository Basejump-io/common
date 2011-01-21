################################################################################
#
# SPLICE Library platform/os/windows/defs.mk makefile
#
################################################################################
#
# $Revision: 14/1 $
#
# Copyright 2009-2011, Qualcomm Innovation Center, Inc.
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
################################################################################

# Multiple include protection
ifeq ($(origin PLATFORM_OS_DEFS_MK), undefined)
PLATFORM_OS_DEFS_MK := 1

#$(info Reading $(lastword $(MAKEFILE_LIST)))


################################################################################
# OS dependent filename affix's

# Binary executable suffix (<nothing> for Linux)
BIN_EXT = .exe


################################################################################
# Build mode defines and setup

# List of supported build modes for the platform
BUILD_MODES = debug release

################################################################################
# Addition options for targets.

endif
