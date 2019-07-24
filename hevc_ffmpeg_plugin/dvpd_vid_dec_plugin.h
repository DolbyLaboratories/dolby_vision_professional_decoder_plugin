/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, Dolby International AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
* @brief video decoder plugin API of the Dolby Vision Pro Decoder SIDK .
* @file dvpd_vid_dec_plugin.h
*
*/


#ifndef __DVPD_VID_DEC_PLUGIN_API_H_
#define __DVPD_VID_DEC_PLUGIN_API_H_

#ifdef _WIN32
	#define DVPD_VID_DEC_PLUGIN_API __declspec(dllexport)
#else
	#define DVPD_VID_DEC_PLUGIN_API
#endif

#include <stdint.h>

#if defined _MSC_VER && _MSC_VER < 1800
typedef int bool;
#define false 0
#define true 1
#else
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

	#define DV_PLUGIN_API_VERSION 0

/*!
	dvpd_input_dec_picture_t
	@brief structure for holding a decoded image.\n
*/
	typedef struct
	{
		uint8_t *data[ 3 ];                 /**< @details pointers to image plane data */
		int32_t stride[ 3 ];                /**< @details image stride per color component */

		int16_t width;                      /**< @details image width */
		int16_t height;                     /**< @details image height */
		int8_t bit_depth;                   /**< @details bitdepth of the image data */

		int64_t pts;                        /**< @details presentation time stamp of this image */
		int64_t dts;                        /**< @details decoding time stamp of this image */
		void *app_specific_data;            /**< @details user owned application specific data which will be passed from the decoded picture of the video decoder
		                                         plugin to the output picture (see also *app_specific_data of dvpd_output_picture_t). This provides the user with 
		                                         a way to access extra information like SPS or PPS  */
	} dvpd_input_dec_picture_t;

	/*!
	dvpd_input_dec_handle_t
	@brief video decoder handle.\n
	*/
	typedef void *dvpd_input_dec_handle_t;

	/*!
	on_decoded_picture_cb_func_t
	@brief
	callback function called when a frame has been successfully decoded.
	@param [in]   dec_picture
	The decoded image
	@param [in]   app_data
	A void pointer to arbitrary application data associated with this frame
	@param [in]   layer
	index of the layer that this image originates from. Can be '0' for base layer or '1' for enhancement layer (in case of a dual-layer Dolby Vision stream).
	*/
	typedef void( *on_decoded_picture_cb_func_t ) (dvpd_input_dec_picture_t *dec_picture, void *app_data, int32_t layer );


	/*!
	dvpd_input_dec_if_t
	@brief
	function interface to the underlying video decoder implementation.
	*/
	typedef struct
	{
		/*!
		create
		@brief
		creates an instance of the underlying video decoder implementation.
		@return handle to the created video decoder instance
		*/
		dvpd_input_dec_handle_t( *create ) ( void );


		/*!
		destroy
		@brief
		destroys the given instance of the underlying video decoder implementation.
		@param [in]  h_dec handle to the video decoder instance to be destroyed.
		*/
		void ( *destroy )(dvpd_input_dec_handle_t* h_dec );


		/*!
		init
		@brief
		destroys the given instance of the underlying video decoder implementation.
		@param [in]  h_dec handle to the video decoder instance to be initialized.
		@param [in]  on_decoded_picture function pointer to the picture callback function.
		@param [in]  app_data pointer to arbitrary application data (attached to each frame during picture callback).
		@param [in]  layer the layer id that this video decoder instance should process.
		@return
			@li = true     success
			@li = false    an error occured
		*/
		bool( *init ) (dvpd_input_dec_handle_t h_dec, on_decoded_picture_cb_func_t on_decoded_picture, void* app_data, int32_t layer );

		/*!
		deinit
		@brief
		deinitializes the given instance of the underlying video decoder implementation.
		@param [in] h_dec handle to the video decoder instance to be deinitialized.
		@return
			@li = true     success
			@li = false    an error occured
		*/
		bool( *deinit ) (dvpd_input_dec_handle_t h_dec );

		/*!
		decode
		@brief
		feeds a bitstream chunk to the underlying video decoder instance.
		@param [in]  h_dec handle to the video decoder instance.
		@param [in]  data pointer to a byte buffer containing the bitstream data to be processed.
		@param [in]  size determines the length of the valid data where the data pointer points to.
		@param [in]  pts presentation timestamp where this data chunk belongs to. 
		             NOTICE: THESE TIMESTAMPS SHOULD NOT BE MODIFIED BY THE DECODER AND THE DECODER MUST SUPPORT 
		             PTS PASS-THROUGH. IF YOUR DECODER DOES NOT SUPPORT PTS PASS-THROUGH YOU SHOULD FIND 
		             ANOTHER WAY TO PASS PTS TIMESTAMPS THROUGH THE DEOCODER, E.G. USER DATA ATTACHED TO EACH PICTURE.
		@param [in]  dts decoding timestamp where this data chunk belongs to.
	                 NOTICE: THESE TIMESTAMPS SHOULD NOT BE MODIFIED BY THE DECODER AND THE DECODER MUST SUPPORT
		             DTS PASS-THROUGH. IF YOUR DECODER DOES NOT SUPPORT DTS PASS-THROUGH YOU SHOULD FIND
		             ANOTHER WAY TO PASS DTS TIMESTAMPS THROUGH THE DEOCODER, E.G. USER DATA ATTACHED TO EACH PICTURE.
		*/
		void( *decode ) (dvpd_input_dec_handle_t h_dec, uint8_t *data, uint32_t size, uint64_t pts, uint64_t dts );


		/*!
		flush
		@brief
		flushes all internal buffers of the underlying video decoder instance.
		@param [in]  h_dec handle to the video decoder instance.
		@param [in]  discard pointer to a byte buffer containing the bitstream data to be processed.
			@li = true     discard all internal buffers. no further output can be expected
			@li = false    decode and output all available images (if any) and flush all internal buffers.
		*/
		void( *flush ) (dvpd_input_dec_handle_t h_dec, bool discard );

		/*!
		is_init
		@brief
		returns the initialization status of the underlying video decoder instance.
		@param [in]  h_dec handle to the video decoder instance to be deinitialized.
		@return
			@li = true     initialized state (ready to use)
			@li = false    non-initialized state (not ready)
		*/
		bool( *is_init ) (dvpd_input_dec_handle_t h_dec );
	} dvpd_input_dec_if_t;


	/*!
	dv_dec_video_dec_plugin_t
	@brief structure for holding a description of the video decoder plugin.\n
	*/
	typedef struct
	{
		int32_t dv_plugin_api_version; // always initialize this to DV_PLUGIN_API_VERSION

		const char *name;              // the plugin's friendly name
		const char *description;       // description about what the plugin does
		const char *copyright;         // your copyright message
		const char *version;           // your version information
		const char *type;              // plugin type: HEVC or AVC
	
		dvpd_input_dec_if_t vid_dec_if;

	} dv_dec_video_dec_plugin_t;

	/*!
	dv_dec_vid_dec_plugin_describe
	@brief fills the provided plugin structure with descriptive information.\n
	@param [in]  dv_dec_video_dec_plugin pointer to a dv_dec_video_dec_plugin_t structure to be filled.
	*/
	DVPD_VID_DEC_PLUGIN_API void dv_dec_vid_dec_plugin_describe( dv_dec_video_dec_plugin_t* dv_dec_video_dec_plugin );

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __DVPD_VID_DEC_PLUGIN_API_H_
