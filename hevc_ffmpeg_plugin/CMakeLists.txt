#
# BSD 3-Clause License
#
# Copyright (c) 2018-2019, Dolby Laboratories
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

cmake_minimum_required(VERSION 3.5)
project(FFmpegHevcDecPlugin)

set(CMAKE_C_STANDARD 99)

FIND_PACKAGE(PkgConfig)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(AVFORMAT libavformat)
	pkg_check_modules(AVCODEC libavcodec)
	pkg_check_modules(AVUTIL libavutil)
endif()

if(NOT AVFORMAT_FOUND OR NOT AVCODEC_FOUND OR NOT AVUTIL_FOUND)
	if(
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg AND
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg/lib/avformat.lib AND
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg/lib/avcodec.lib AND
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg/lib/avutil.lib AND
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg/include/libavformat/avformat.h AND
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg/include/libavcodec/avcodec.h AND
		EXISTS ${PROJECT_SOURCE_DIR}/ffmpeg/include/libavutil/avutil.h)

		SET(AVFORMAT_FOUND TRUE)
		SET(AVCODEC_FOUND TRUE)
		SET(AVUTIL_FOUND TRUE)

		SET(AVFORMAT_INCLUDE_DIRS ffmpeg/include)
		SET(AVCODEC_INCLUDE_DIRS ffmpeg/include)
		SET(AVUTIL_INCLUDE_DIRS ffmpeg/include)

		SET(AVFORMAT_LIBRARY_DIRS ffmpeg/lib)
		SET(AVCODEC_LIBRARY_DIRS ffmpeg/lib)
		SET(AVUTIL_LIBRARY_DIRS ffmpeg/lib)

		SET(AVFORMAT_LIBRARIES avformat.lib)
		SET(AVCODEC_LIBRARIES avcodec.lib)
		SET(AVUTIL_LIBRARIES avutil.lib)

		MESSAGE("Found FFmpeg libraries in 'ffmpeg' directory.")
	else()
		MESSAGE("Did not find all FFmpeg libraries and header files in 'ffmpeg' directory.")
	endif()
endif()

if(AVFORMAT_FOUND AND AVCODEC_FOUND AND AVUTIL_FOUND)
	link_directories(${AVFORMAT_LIBRARY_DIRS} ${AVCODEC_LIBRARY_DIRS} ${AVUTIL_LIBRARY_DIRS})
	add_library(FFmpegDecPlugin SHARED ffmpeg_hevc_dec_plugin.c)
	target_include_directories(FFmpegDecPlugin PRIVATE ${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS})
	target_link_libraries(FFmpegDecPlugin PRIVATE ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES})
else()
	MESSAGE(FATAL_ERROR "Could not create build files for ffmpeg HEVC decoder plug-in.")
endif()