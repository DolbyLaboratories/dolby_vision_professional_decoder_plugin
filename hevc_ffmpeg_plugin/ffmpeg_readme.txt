Building the sample FFmpeg decoder plug-in

Before you build the plug-in, check out the Dolby GitHub repository at https://github.com/
DolbyLaboratories/dolby_vision_professional_decoder_plugin. The FFmpeg files you need are contained in
the hevc_ffmpeg_plugin directory.

1.1 Building the plug-in on Microsoft Windows
These steps describe how to build a decoder plug-in on Microsoft Windows.

Prerequisites
Before you begin building the sample FFmpeg decoder, download the FFmpeg source files and build the binaries.

Procedure
	1. inside the 'dolbyvision_professional_decoder\hevc_ffmpeg_plugin' folder create a new directory .\ffmpeg 
	2. For every C header and library file in the FFmpeg package, copy files from FFmpeg to this directory as shown:
		• ffmpeg\lib\avcodec.lib
		• ffmpeg\lib\avutil.lib
		• ffmpeg\include\libavcodec	
		• ffmpeg\include\libavformat
		• ffmpeg\include\libavutil
	3. In the 'dolbyvision_professional_decoder\hevc_ffmpeg_plugin', create a 'build' directory.
	4. Change to the build directory and open a command-line window.
	5. Use CMake to build the files. Specify the platform and toolchain. This example uses a Microsoft Visual Studio toolchain and x64 architecture.

		cmake .. -A x64 -T v140

	6. Build the sample plug-in using either the command line or an integrated development environment.
		• On the command line, specify either a debug version or a release version.
			cmake --build . --config Debug
			cmake --build . --config Release
		• In the integrated development environment, open the ffmpeg_hevc_dec_plugin.sln solution file and build the sample plug-in there.
		
	The FFmpegDecPlugin.dll file is created.

1.2 Building the plug-in on Linux
These steps describe how to build a decoder plug-in on Linux.

Procedure
	1. Import the required headers and libraries using the pkg-config tool.
	2. Install the specific FFmpeg packages using your distribution package manager as shown:
		• ffmpeg\lib\avcodec.lib
		• ffmpeg\lib\avutil.lib
		• ffmpeg\include\libavcodec
		• ffmpeg\include\libavformat
		• ffmpeg\include\libavutil
	3. In the ./examples/hevcDecPlugin/ffmpeg directory, create a build directory.
	4. Change to the build directory and open a command-line window.
	5. Use CMake to build the files. Specify the build type.
		• cmake .. -DCMAKE_BUILD_TYPE=Debug
		• cmake .. -DCMAKE_BUILD_TYPE=Release
	6. Then build the sample plug-in files.

		cmake --build.
		
	The FFmpegDecPlugin.so file is created.