/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2018-2019, Dolby Laboratories
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

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#define inline __inline
#else
#include <unistd.h>
#endif

#include "dvpd_vid_dec_plugin.h"
#include <libavcodec/avcodec.h>

typedef void* ffmpeg_vid_dec_handle;

typedef struct ffmpeg_vid_dec_ctx_s_
{
	const AVCodec *codec;
	AVCodecContext *av_codec_ctx;
	AVPacket *pkt;
	AVFrame *frame;

	bool init;

	on_decoded_picture_cb_func_t on_decoded_picture;
	void* app_data;
	int32_t layer;
} ffmpeg_vid_dec_ctx_t;

static int get_cpu_count() {
	int i_num_cores = 1;
#if defined(_WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	i_num_cores = sysinfo.dwNumberOfProcessors;
#else
	i_num_cores = (int)sysconf( _SC_NPROCESSORS_ONLN );
#endif
	if (i_num_cores < 1)
	{
		i_num_cores = 1;
	}

	if (i_num_cores > 16)
	{
		i_num_cores = 16;
	}

	return i_num_cores;
}

static ffmpeg_vid_dec_handle ffmpeg_vid_dec_create( void )
{
	ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx = ( ffmpeg_vid_dec_ctx_t* )malloc( sizeof( ffmpeg_vid_dec_ctx_t ) );

	if( ffmpeg_vid_dec_ctx == NULL )
	{
		return NULL;
	}

	memset( ffmpeg_vid_dec_ctx, 0, sizeof( ffmpeg_vid_dec_ctx_t ) );

	return ffmpeg_vid_dec_ctx;
}

static void ffmpeg_vid_dec_destroy( ffmpeg_vid_dec_handle* h_dec )
{
	ffmpeg_vid_dec_ctx_t* ctx = ( ffmpeg_vid_dec_ctx_t* )*h_dec;

	free( ctx );
	h_dec = NULL;
}

static bool ffmpeg_vid_dec_init( ffmpeg_vid_dec_handle h_dec, on_decoded_picture_cb_func_t on_decoded_picture, void* app_data, int32_t layer )
{
	ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx = ( ffmpeg_vid_dec_ctx_t* )h_dec;

	if( ffmpeg_vid_dec_ctx == NULL )
	{
		return false;
	}

	if( ffmpeg_vid_dec_ctx->init == true )
	{
		return false;
	}

	ffmpeg_vid_dec_ctx->on_decoded_picture = on_decoded_picture;
	ffmpeg_vid_dec_ctx->app_data = app_data;
	ffmpeg_vid_dec_ctx->layer = layer;

#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,9,100) )
	avcodec_register_all( );
#endif

#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,101) )
	ffmpeg_vid_dec_ctx->pkt = malloc( sizeof( AVPacket ) );
	if( ffmpeg_vid_dec_ctx->pkt == NULL )
	{
		goto bail;
	}
#else
	ffmpeg_vid_dec_ctx->pkt = av_packet_alloc( );
	if( ffmpeg_vid_dec_ctx->pkt == NULL )
	{
		goto bail;
	}
#endif

#ifdef AVC_CODEC
	ffmpeg_vid_dec_ctx->codec = avcodec_find_decoder( AV_CODEC_ID_H264 );
#else
	ffmpeg_vid_dec_ctx->codec = avcodec_find_decoder( AV_CODEC_ID_H265 );
#endif
	if( ffmpeg_vid_dec_ctx->codec == NULL )
	{
		goto bail;
	}

	ffmpeg_vid_dec_ctx->av_codec_ctx = avcodec_alloc_context3( ffmpeg_vid_dec_ctx->codec );
	if( ffmpeg_vid_dec_ctx->av_codec_ctx == NULL )
	{
		goto bail;
	}

	ffmpeg_vid_dec_ctx->av_codec_ctx->thread_count = get_cpu_count();

	if( avcodec_open2( ffmpeg_vid_dec_ctx->av_codec_ctx, ffmpeg_vid_dec_ctx->codec, NULL ) < 0 )
	{
		goto bail;
	}

	ffmpeg_vid_dec_ctx->frame = av_frame_alloc( );
	if( ffmpeg_vid_dec_ctx->frame == NULL )
	{
		goto bail;
	}

	ffmpeg_vid_dec_ctx->init = true;

	return true;

bail:
	if( ffmpeg_vid_dec_ctx->av_codec_ctx != NULL )
	{
		avcodec_free_context( &ffmpeg_vid_dec_ctx->av_codec_ctx );
	}

	if( ffmpeg_vid_dec_ctx->frame != NULL )
	{
		av_frame_free( &ffmpeg_vid_dec_ctx->frame );
	}

	if( ffmpeg_vid_dec_ctx->pkt != NULL )
	{
#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,101) )
		free( ffmpeg_vid_dec_ctx->pkt );
#else
		av_packet_free( &ffmpeg_vid_dec_ctx->pkt );
#endif
	}

	return false;
}

static bool ffmpeg_vid_dec_deinit( ffmpeg_vid_dec_handle h_dec )
{
	ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx = ( ffmpeg_vid_dec_ctx_t* )h_dec;

	if( ffmpeg_vid_dec_ctx == NULL )
	{
		return false;
	}

	if( ffmpeg_vid_dec_ctx->init == false )
	{
		return false;
	}

	avcodec_free_context( &ffmpeg_vid_dec_ctx->av_codec_ctx );
	av_frame_free( &ffmpeg_vid_dec_ctx->frame );
#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,101) )
		free( ffmpeg_vid_dec_ctx->pkt );
#else
		av_packet_free( &ffmpeg_vid_dec_ctx->pkt );
#endif

	ffmpeg_vid_dec_ctx->init = false;

	return true;
}

static void decode( ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx, AVPacket* avpkt )
{
#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,101) )
	while( 1 )
	{
		dvpd_input_dec_picture_t output_picture = {0};
		AVFrame* output_frame = ffmpeg_vid_dec_ctx->frame;
		int got_picture = 0;

		int32_t ret = avcodec_decode_video2( ffmpeg_vid_dec_ctx->av_codec_ctx, output_frame, &got_picture, avpkt );
		if( ret < 0 )
		{
			// fprintf( stderr, "Error sending a packet for decoding\n" );
			return;
		}

		avpkt->size -= ret;
		avpkt->data += ret;

		if( avpkt->size == 0 && got_picture == 0 )
		{
			return;
		}

		if( got_picture == 0 )
		{
			continue;
		}
#else
	int32_t ret = avcodec_send_packet( ffmpeg_vid_dec_ctx->av_codec_ctx, avpkt );
	if( ret < 0 )
	{
		// fprintf( stderr, "Error sending a packet for decoding\n" );
		return;
	}

	while( ret >= 0 )
	{
		dvpd_input_dec_picture_t output_picture = {0};
		AVFrame* output_frame = ffmpeg_vid_dec_ctx->frame;

		ret = avcodec_receive_frame( ffmpeg_vid_dec_ctx->av_codec_ctx, output_frame );
		if( ret == AVERROR( EAGAIN ) || ret == AVERROR_EOF )
		{
			return;
		}
		else if( ret < 0 )
		{
			// fprintf( stderr, "Error during decoding\n" );
			return;
		}
#endif

		switch( ( enum AVPixelFormat )output_frame->format )
		{
		case AV_PIX_FMT_YUV420P:
			output_picture.bit_depth = 8;
			break;
		case AV_PIX_FMT_YUV420P10LE:
			output_picture.bit_depth = 10;
			break;
		default:
			// fprintf( stderr, "Error! raw output format  not supported\n" );
			return;
		}

		for( int32_t i = 0; i < 3; i++ )
		{
			output_picture.data[ i ] = output_frame->data[ i ];
			output_picture.stride[ i ] = output_frame->linesize[ i ];
		}
		output_picture.width = output_frame->width;
		output_picture.height = output_frame->height;
		output_picture.dts = output_frame->pkt_dts;
		output_picture.app_specific_data = NULL;
#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,101) )
		output_picture.pts = output_frame->pkt_pts;
#else
		output_picture.pts = output_frame->pts;
#endif

		ffmpeg_vid_dec_ctx->on_decoded_picture( &output_picture, ffmpeg_vid_dec_ctx->app_data, ffmpeg_vid_dec_ctx->layer );
	}

	return;
}


static void ffmpeg_vid_dec_decode( ffmpeg_vid_dec_handle h_dec, uint8_t *data, uint32_t size, uint64_t pts, uint64_t dts )
{
	ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx = ( ffmpeg_vid_dec_ctx_t* )h_dec;

	if( ffmpeg_vid_dec_ctx == NULL )
	{
		return;
	}

	if( ffmpeg_vid_dec_ctx->init == false )
	{
		return;
	}

	av_init_packet( ffmpeg_vid_dec_ctx->pkt );

	ffmpeg_vid_dec_ctx->pkt->data = data;
	ffmpeg_vid_dec_ctx->pkt->size = size;
	ffmpeg_vid_dec_ctx->pkt->dts = dts;
	ffmpeg_vid_dec_ctx->pkt->pts = pts;

	decode( ffmpeg_vid_dec_ctx, ffmpeg_vid_dec_ctx->pkt );

	return;
}

static void ffmpeg_vid_dec_flush( ffmpeg_vid_dec_handle h_dec, bool discard )
{
	ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx = ( ffmpeg_vid_dec_ctx_t* )h_dec;

	if( ffmpeg_vid_dec_ctx == NULL )
	{
		return;
	}

	if( ffmpeg_vid_dec_ctx->init == false )
	{
		return;
	}

	if( discard == false )
	{
#if ( LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,101) )
		av_init_packet( ffmpeg_vid_dec_ctx->pkt );
		ffmpeg_vid_dec_ctx->pkt->data = NULL;
		ffmpeg_vid_dec_ctx->pkt->size = 0;
		decode( ffmpeg_vid_dec_ctx, ffmpeg_vid_dec_ctx->pkt );
#else
		decode( ffmpeg_vid_dec_ctx, NULL );
#endif
	}

	avcodec_flush_buffers( ffmpeg_vid_dec_ctx->av_codec_ctx );

	return;
}

static bool ffmpeg_vid_dec_is_init(dvpd_input_dec_handle_t h_dec )
{
	ffmpeg_vid_dec_ctx_t* ffmpeg_vid_dec_ctx = ( ffmpeg_vid_dec_ctx_t* )h_dec;

	if( ffmpeg_vid_dec_ctx == NULL )
	{
		return false;
	}

	return ffmpeg_vid_dec_ctx->init;
}

DVPD_VID_DEC_PLUGIN_API void dv_dec_vid_dec_plugin_describe( dv_dec_video_dec_plugin_t* dv_dec_video_dec_plugin )
{
	dv_dec_video_dec_plugin->dv_plugin_api_version = DV_PLUGIN_API_VERSION;
#ifdef AVC_CODEC
	dv_dec_video_dec_plugin->name = "FFmpeg AVC decoder";
	dv_dec_video_dec_plugin->type = "AVC";
	dv_dec_video_dec_plugin->description = "FFmpeg AVC decoder plugin for Dolby Vision Professional Decoder SIDK";
	dv_dec_video_dec_plugin->copyright = "Dolby Laboratories (c) 2019, BSD3";
	dv_dec_video_dec_plugin->version = "ver0.1";
#else
	dv_dec_video_dec_plugin->name = "FFmpeg HEVC decoder";
	dv_dec_video_dec_plugin->type = "HEVC";
	dv_dec_video_dec_plugin->description = "FFmpeg HEVC decoder plugin for Dolby Vision Professional Decoder SIDK";
	dv_dec_video_dec_plugin->copyright = "Dolby Laboratories (c) 2019, BSD3";
	dv_dec_video_dec_plugin->version = "ver0.1";
#endif

	dv_dec_video_dec_plugin->vid_dec_if.create = ffmpeg_vid_dec_create;
	dv_dec_video_dec_plugin->vid_dec_if.decode = ffmpeg_vid_dec_decode;
	dv_dec_video_dec_plugin->vid_dec_if.deinit = ffmpeg_vid_dec_deinit;
	dv_dec_video_dec_plugin->vid_dec_if.destroy = ffmpeg_vid_dec_destroy;
	dv_dec_video_dec_plugin->vid_dec_if.flush = ffmpeg_vid_dec_flush;
	dv_dec_video_dec_plugin->vid_dec_if.init = ffmpeg_vid_dec_init;
	dv_dec_video_dec_plugin->vid_dec_if.is_init = ffmpeg_vid_dec_is_init;
}
