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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideodecoder.h>
#include "gstdvprodecoder.h"

GST_DEBUG_CATEGORY_STATIC (gst_dvprodecoder_debug_category);
#define GST_CAT_DEFAULT gst_dvprodecoder_debug_category

/* prototypes */


static void gst_dvprodecoder_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_dvprodecoder_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static gboolean gst_dvprodecoder_start (GstVideoDecoder * decoder);
static gboolean gst_dvprodecoder_stop (GstVideoDecoder * decoder);
static gboolean gst_dvprodecoder_flush (GstVideoDecoder * decoder);
static GstFlowReturn gst_dvprodecoder_finish (GstVideoDecoder * decoder);
static GstFlowReturn gst_dvprodecoder_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);

enum
{
  PROP_0,
  PROP_PROFILE,
  PROP_OUTMODE,
  PROP_ALGO_VERSION,
  PROP_HEVC_PLUGIN,
};

#define GST_TYPE_DVPRODECODER_PROFILE (gst_dvprodecoder_profile_get_type ())
static GType
gst_dvprodecoder_profile_get_type (void)
{
  static GType dvprodecoder_profile_type = 0;

  if (!dvprodecoder_profile_type) {
    static GEnumValue profile_types[] = {
      { DVPD_INPUT_DV_PROFILE_4, "Profile 4 - dvhe.dtr", "profile4" },
      { DVPD_INPUT_DV_PROFILE_5, "Profile 5 - dvhe.stn", "profile5" },
   //   { DVPD_INPUT_DV_PROFILE_7, "Profile 7 - dvhe.dtb", "profile7" },
      { DVPD_INPUT_DV_PROFILE_8, "Profile 8 - dvhe.st",  "profile8" },
      { 0, NULL, NULL },
    };

    dvprodecoder_profile_type =
    g_enum_register_static ("GstDvprodecoderProfile",
                profile_types);
  }

  return dvprodecoder_profile_type;
}

#define GST_TYPE_DVPRODECODER_OUTPUTMODE (gst_dvprodecoder_outputmode_get_type ())
static GType
gst_dvprodecoder_outputmode_get_type (void)
{
  static GType dvprodecoder_outputmode_type = 0;

  if (!dvprodecoder_outputmode_type) {
    static GEnumValue outputmode_types[] = {
      { DOLBY_VISION_NATIVE, "as GST_VIDEO_FORMAT_I420_12LE", "dolby_vision_native" },
      { CSC_ITP_420P_FULL_12, "", "csc_itp_420p_full_12" },
      { CSC_RGB_P3D65_FULL_10, "as GST_VIDEO_FORMAT_GBR_10LE", "csc_rgb_p3d65_full_10" },
      { CSC_RGB_P3D65_FULL_12, "as GST_VIDEO_FORMAT_GBR_12LE", "csc_rgb_p3d65_full_12" },
      { CSC_RGB_BT2100_FULL_10, "as GST_VIDEO_FORMAT_GBR_10LE", "csc_rgb_bt2100_full_10" },
      { CSC_RGB_BT2100_FULL_12, "as GST_VIDEO_FORMAT_GBR_12LE", "csc_rgb_bt2100_full_12" },
      { CSC_YUV_P3D65_420P_NARROW_10, "", "csc_yuv_p3d65_420p_narrow_10" },
      { CSC_YUV_P3D65_420P_NARROW_12, "", "csc_yuv_p3d65_420p_narrow_12" },
      { CSC_YUV_P3D65mat709_420P_NARROW_10, "", "csc_yuv_p3d65mat709_420p_narrow_10" },
      { CSC_YUV_P3D65mat709_420P_NARROW_12, "", "csc_yuv_p3d65mat709_420p_narrow_12" },
      { CSC_YUV_BT2100_420P_NARROW_10, "", "csc_yuv_bt2100_420p_narrow_10" },
      { CSC_YUV_BT2100_420P_NARROW_12, "", "csc_yuv_bt2100_420p_narrow_12" },
      { DOLBY_VISION_HDMI, "as GST_VIDEO_FORMAT_I422_12LE", "dolby_vision_hdmi" },
      { DM_SDR100_BT709_8, "", "dm_sdr100_bt709_8" },
      { DM_SDR100_BT709_10, "", "dm_sdr100_bt709_10" },
      { DM_HDR600_BT2100_10, "", "dm_hdr600_bt2100_10" },
      { DM_HDR600_BT2100_12, "", "dm_hdr600_bt2100_12" },
      { DM_HDR1000_BT2100_10, "", "dm_hdr1000_bt2100_10" },
      { DM_HDR1000_BT2100_12, "", "dm_hdr1000_bt2100_12" },
      { 0, NULL, NULL },
    };

    dvprodecoder_outputmode_type =
    g_enum_register_static ("GstDvprodecoderOutputmode",
                outputmode_types);
  }

  return dvprodecoder_outputmode_type;
}


#define GST_TYPE_DVPRODECODER_ALGO (gst_dvprodecoder_algo_get_type ())
static GType
gst_dvprodecoder_algo_get_type (void)
{
  static GType dvprodecoder_algo_type = 0;

  if (!dvprodecoder_algo_type) {
    static GEnumValue algo_types[] = {
      { DVPD_DM_VER3, "DM version 3 algorithm", "dm3" },
      { DVPD_DM_VER4, "DM version 4 algorithm", "dm4" },
      { 0, NULL, NULL },
    };

    dvprodecoder_algo_type =
    g_enum_register_static ("GstDvprodecoderAlgo",
                algo_types);
  }

  return dvprodecoder_algo_type;
}

/* pad templates */

static GstStaticPadTemplate gst_dvprodecoder_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h265, alignment=au, stream-format=byte-stream")
    );

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ I420, I420_10LE, I420_12LE, Y444_12LE, BGR_10LE, BGR_12LE, I422_12LE }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstDvprodecoder, gst_dvprodecoder, GST_TYPE_VIDEO_DECODER,
  GST_DEBUG_CATEGORY_INIT (gst_dvprodecoder_debug_category, "dvprodecoder", 0,
  "debug category for dvprodecoder element"));

static gint find_frame_pts(gconstpointer a, gconstpointer b)
{
  GstVideoCodecFrame* frame = (GstVideoCodecFrame*)a;
  dvpd_output_picture_t* decoded_frame = (dvpd_output_picture_t*)b;

  if (decoded_frame->pts == GST_CLOCK_TIME_NONE || frame->pts == GST_CLOCK_TIME_NONE)
  {
    return 1;
  }

  return (int64_t)decoded_frame->pts - frame->pts;
}

static gint find_frame_dts(gconstpointer a, gconstpointer b)
{
  GstVideoCodecFrame* frame = (GstVideoCodecFrame*)a;
  dvpd_output_picture_t* decoded_frame = (dvpd_output_picture_t*)b;

  if (decoded_frame->dts == GST_CLOCK_TIME_NONE || frame->dts == GST_CLOCK_TIME_NONE)
  {
    return 1;
  }

  return (int64_t)decoded_frame->dts - frame->dts;
}

static void unref_frame(gpointer frame)
{
  gst_video_codec_frame_unref(frame);
}

static void on_output_picture_cb_func(void* user, dvpd_output_picture_t* output_picture)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (user);

  GST_DEBUG_OBJECT (dvprodecoder, "on_output_picture_cb_func");

  GstVideoCodecState *state = gst_video_decoder_get_output_state(GST_VIDEO_DECODER(dvprodecoder));
  if (state == NULL)
  {
    GstVideoFormat fmt;

    switch (dvprodecoder->output_mode)
    {
      case DOLBY_VISION_NATIVE:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;

      case CSC_ITP_420P_FULL_12:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;

      case CSC_RGB_P3D65_FULL_10:
        fmt = GST_VIDEO_FORMAT_GBR_10LE;
        break;
      case CSC_RGB_P3D65_FULL_12:
        fmt = GST_VIDEO_FORMAT_GBR_12LE;
        break;
      case CSC_RGB_BT2100_FULL_10:
        fmt = GST_VIDEO_FORMAT_GBR_10LE;
        break;
      case CSC_RGB_BT2100_FULL_12:
        fmt = GST_VIDEO_FORMAT_GBR_12LE;
        break;

      case CSC_YUV_P3D65_420P_NARROW_10:
        fmt = GST_VIDEO_FORMAT_I420_10LE;
        break;
      case CSC_YUV_P3D65_420P_NARROW_12:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;
      case CSC_YUV_P3D65mat709_420P_NARROW_10:
        fmt = GST_VIDEO_FORMAT_I420_10LE;
        break;
      case CSC_YUV_P3D65mat709_420P_NARROW_12:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;
      case CSC_YUV_BT2100_420P_NARROW_10:
        fmt = GST_VIDEO_FORMAT_I420_10LE;
        break;
      case CSC_YUV_BT2100_420P_NARROW_12:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;

      case DOLBY_VISION_HDMI:
        fmt = GST_VIDEO_FORMAT_I422_12LE;
        break;

      case DM_SDR100_BT709_8:
        fmt = GST_VIDEO_FORMAT_I420;
        break;
      case DM_SDR100_BT709_10:
        fmt = GST_VIDEO_FORMAT_I420_10LE;
        break;
      case DM_HDR600_BT2100_10:
        fmt = GST_VIDEO_FORMAT_I420_10LE;
        break;
      case DM_HDR600_BT2100_12:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;
      case DM_HDR1000_BT2100_10:
        fmt = GST_VIDEO_FORMAT_I420_10LE;
        break;
      case DM_HDR1000_BT2100_12:
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;
      default:
        g_warning("FIXME: unhandled format..");
        fmt = GST_VIDEO_FORMAT_I420_12LE;
        break;
    }
    gst_video_decoder_set_output_state(GST_VIDEO_DECODER(dvprodecoder), fmt, output_picture->width, output_picture->height, NULL);
  }
  else
  {
    gst_video_codec_state_unref(state);
  }

  GstVideoCodecFrame* frame = NULL;

  GList* frames = gst_video_decoder_get_frames(GST_VIDEO_DECODER(dvprodecoder));

  // find a frame with matching PTS
  GList* tmp = g_list_find_custom(frames, output_picture, find_frame_pts);
  if (tmp != NULL)
  {
    frame = gst_video_codec_frame_ref((GstVideoCodecFrame*)tmp->data);
  }

  // find a frame with matching DTS if none with matching PTS was found
  if (frame == NULL)
  {
    GList* tmp = g_list_find_custom(frames, output_picture, find_frame_dts);
    if (tmp != NULL)
    {
      frame = gst_video_codec_frame_ref((GstVideoCodecFrame*)tmp->data);
    }
  }

  g_list_free_full(frames, unref_frame);

  // We didn't find any good match before. As a last resort just get the oldest frame
  // and hope we still find one.
  if (frame == NULL)
  {
    frame = gst_video_decoder_get_oldest_frame(GST_VIDEO_DECODER(dvprodecoder));
    g_assert(frame != NULL);

    frame->pts = GST_CLOCK_TIME_NONE;
    frame->dts = output_picture->dts;
  }

  gst_video_decoder_allocate_output_frame(GST_VIDEO_DECODER(dvprodecoder), frame);
  gst_buffer_fill(frame->output_buffer, 0, output_picture->frame_data, output_picture->data_size);

  gst_video_decoder_finish_frame(GST_VIDEO_DECODER(dvprodecoder), frame);
}

static int32_t on_notification_cb_func(void *user, dvpd_notification_type type, const char *message)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (user);

  GST_DEBUG_OBJECT (dvprodecoder, "on_notification_cb_func: type -> %d, msg -> %s", type, message);

  /*
   * TODO: send error messages etc to bus instead of hard erroring here.
   */
  if (type == DVPD_NOTIFICATION_PANIC)
  {
    g_error("%s", message);
  }
  else if (type == DVPD_NOTIFICATION_ERROR)
  {
    g_warning("%s", message);
  }

  return 1;
}

static void
gst_dvprodecoder_class_init (GstDvprodecoderClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_dvprodecoder_sink_template);
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SRC_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Dolby Vision Professional Decoder", "Generic", "Dolby Vision Professional Decoder",
      "Dolby Laboratories <contact@dolby.com>");

  gobject_class->set_property = gst_dvprodecoder_set_property;
  gobject_class->get_property = gst_dvprodecoder_get_property;
  video_decoder_class->start = GST_DEBUG_FUNCPTR (gst_dvprodecoder_start);
  video_decoder_class->stop = GST_DEBUG_FUNCPTR (gst_dvprodecoder_stop);
  video_decoder_class->flush = GST_DEBUG_FUNCPTR (gst_dvprodecoder_flush);
  video_decoder_class->finish = GST_DEBUG_FUNCPTR (gst_dvprodecoder_finish);
  video_decoder_class->handle_frame = GST_DEBUG_FUNCPTR (gst_dvprodecoder_handle_frame);

  g_object_class_install_property (gobject_class, PROP_PROFILE,
    g_param_spec_enum ("input-mode", "Dolby Vision Profile",
              "Set the Dolby Vision Profile of the input stream",
              GST_TYPE_DVPRODECODER_PROFILE, DVPD_INPUT_DV_PROFILE_5,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OUTMODE,
    g_param_spec_enum ("output-mode", "Output mode",
              "Set the output mode; see dvpd_output_mode_t as reference",
              GST_TYPE_DVPRODECODER_OUTPUTMODE, DM_SDR100_BT709_8,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_ALGO_VERSION,
    g_param_spec_enum ("dm-version", "DM algorithm version",
              "Set the DM algorithm version; see dvpd_dm_algo_t as reference",
              GST_TYPE_DVPRODECODER_ALGO, DVPD_DM_VER3,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_HEVC_PLUGIN,
    g_param_spec_string ("hevc-plugin", "HEVC decoder plugin",
              "Location of the HEVC decoder plugin",
              NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_dvprodecoder_init (GstDvprodecoder *dvprodecoder)
{
  dvprodecoder->ctx = NULL;
  memset(&dvprodecoder->cfg, 0, sizeof(dvpd_config_t));

#if defined _WIN32
  g_snprintf(dvprodecoder->hevc_plugin_name, sizeof(dvprodecoder->hevc_plugin_name), "%s", "DlbHevcDecPlugin.dll");
#elif defined __APPLE__
  g_snprintf(dvprodecoder->hevc_plugin_name, sizeof(dvprodecoder->hevc_plugin_name), "%s", "libDlbHevcDecPlugin.dylib");
#else
  g_snprintf(dvprodecoder->hevc_plugin_name, sizeof(dvprodecoder->hevc_plugin_name), "%s", "libDlbHevcDecPlugin.so");
#endif

  dvprodecoder->cfg.on_output_picture = on_output_picture_cb_func;
  dvprodecoder->cfg.on_notify = on_notification_cb_func;
  dvprodecoder->cfg.user_data = dvprodecoder;
  dvprodecoder->cfg.input_mode = DVPD_INPUT_DV_PROFILE_5;
  dvprodecoder->cfg.hevc_dec_plugin_name = dvprodecoder->hevc_plugin_name;
  dvprodecoder->output_mode = DM_SDR100_BT709_8;

  dvpd_get_output_config(&dvprodecoder->cfg.output_config, dvprodecoder->output_mode);
}

void
gst_dvprodecoder_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (object);

  GST_DEBUG_OBJECT (dvprodecoder, "set_property");

  switch (property_id) {
    case PROP_PROFILE:
      dvprodecoder->cfg.input_mode = g_value_get_enum(value);
      break;
    case PROP_OUTMODE:
      dvprodecoder->output_mode = g_value_get_enum(value);
      // update the DM values to new setting
      dvpd_get_output_config(&dvprodecoder->cfg.output_config, dvprodecoder->output_mode);

      // overwrite defaults to better match avaiable output formats
      switch (dvprodecoder->output_mode)
      {
        case DOLBY_VISION_HDMI:
          dvprodecoder->cfg.output_config.arrangement = DM_PLANAR_422;
          dvprodecoder->cfg.output_config.dm_metadata_embedding = false;
          break;
        case CSC_RGB_P3D65_FULL_10:
        case CSC_RGB_P3D65_FULL_12:
        case CSC_RGB_BT2100_FULL_10:
        case CSC_RGB_BT2100_FULL_12:
          dvprodecoder->cfg.output_config.arrangement = DM_PLANAR_444;
          break;
        default:
          break;
      }

      break;
    case PROP_ALGO_VERSION:
      dvprodecoder->cfg.output_config.algo = g_value_get_enum(value);
      break;
    case PROP_HEVC_PLUGIN:
      g_snprintf(dvprodecoder->hevc_plugin_name, sizeof(dvprodecoder->hevc_plugin_name), "%s", g_value_get_string(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_dvprodecoder_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (object);

  GST_DEBUG_OBJECT (dvprodecoder, "get_property");

  switch (property_id) {
    case PROP_PROFILE:
      g_value_set_enum(value, dvprodecoder->cfg.input_mode);
      break;
    case PROP_OUTMODE:
      g_value_set_enum(value, dvprodecoder->output_mode);
      break;
    case PROP_ALGO_VERSION:
      g_value_set_enum(value, dvprodecoder->cfg.output_config.algo);
      break;
    case PROP_HEVC_PLUGIN:
      g_value_set_string(value, dvprodecoder->hevc_plugin_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static gboolean
gst_dvprodecoder_start (GstVideoDecoder * decoder)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (decoder);

  GST_DEBUG_OBJECT (dvprodecoder, "start");

  dvprodecoder->ctx = dvpd_create();
  dvpd_init(dvprodecoder->ctx, &dvprodecoder->cfg);

  return TRUE;
}

static gboolean
gst_dvprodecoder_stop (GstVideoDecoder * decoder)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (decoder);

  GST_DEBUG_OBJECT (dvprodecoder, "stop");

  dvpd_reset(dvprodecoder->ctx);
  dvpd_deinit(dvprodecoder->ctx);
  dvpd_destroy(&dvprodecoder->ctx);

  return TRUE;
}

static gboolean
gst_dvprodecoder_flush (GstVideoDecoder * decoder)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (decoder);

  GST_DEBUG_OBJECT (dvprodecoder, "flush");

  GST_VIDEO_DECODER_STREAM_UNLOCK(dvprodecoder);

  dvpd_reset (dvprodecoder->ctx);

  GST_VIDEO_DECODER_STREAM_LOCK(dvprodecoder);

  return TRUE;
}

static GstFlowReturn
gst_dvprodecoder_finish (GstVideoDecoder * decoder)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (decoder);

  GST_DEBUG_OBJECT (dvprodecoder, "finish");

  GST_VIDEO_DECODER_STREAM_UNLOCK(dvprodecoder);

  dvpd_push(dvprodecoder->ctx, DVPD_VES_LAYER_BASE, NULL, 0, INT64_MIN, INT64_MIN);

  dvpd_join(dvprodecoder->ctx);

  GST_VIDEO_DECODER_STREAM_LOCK(dvprodecoder);

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_dvprodecoder_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstDvprodecoder *dvprodecoder = GST_DVPRODECODER (decoder);

  GST_DEBUG_OBJECT (dvprodecoder, "handle_frame");

  GstMapInfo info;

  gst_buffer_map(frame->input_buffer, &info, GST_MAP_READ);

  GST_VIDEO_DECODER_STREAM_UNLOCK(dvprodecoder);

  dvpd_push(dvprodecoder->ctx, DVPD_VES_LAYER_BASE, info.data, info.size, GST_BUFFER_PTS(frame->input_buffer), GST_BUFFER_DTS(frame->input_buffer));

  GST_VIDEO_DECODER_STREAM_LOCK(dvprodecoder);

  gst_buffer_unmap(frame->input_buffer, &info);
  gst_video_codec_frame_unref(frame);

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "dvprodecoder", GST_RANK_NONE,
      GST_TYPE_DVPRODECODER);
}

#ifndef VERSION
#define VERSION "0.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "dvprodecoder"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "Dolby Vision Professional Decoder"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://dolby.com/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dvprodecoder,
    "Dolby Vision Professional Decoder",
    plugin_init, VERSION, "Proprietary", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

