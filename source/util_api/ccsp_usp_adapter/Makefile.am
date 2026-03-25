##########################################################################
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
AM_CXXFLAGS = -std=c++17

ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libCcspCommonLibrary_ccsp_usp_adapter.la

libCcspCommonLibrary_ccsp_usp_adapter_la_CPPFLAGS = \
	-I$(top_srcdir)/source/ccsp/include \
	-I$(top_srcdir)/source/ccsp/components/include \
	-I$(top_srcdir)/source/ccsp/custom \
	-I$(top_srcdir)/source/cosa/include \
	-I$(top_srcdir)/source/cosa/include/linux \
	-I$(top_srcdir)/source/debug_api/include \
	-I$(top_srcdir)/source/util_api/ccsp_usp_adapter

libCcspCommonLibrary_ccsp_usp_adapter_la_SOURCES = \
	ccsp_usp_types.cpp \
	ccsp_usp_provider_adapter.cpp \
	ccsp_usp_consumer_adapter.cpp \
	ccsp_usp_subscription_adapter.cpp

libCcspCommonLibrary_ccsp_usp_adapter_la_LDFLAGS = -lusp -lpthread
