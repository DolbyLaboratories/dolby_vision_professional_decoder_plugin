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

#ifndef _GST_DVPRODECODER_H_
#define _GST_DVPRODECODER_H_

#include <gst/video/video.h>
#include <gst/video/gstvideodecoder.h>
#include "dvpd_api.h"

G_BEGIN_DECLS

#define GST_TYPE_DVPRODECODER   (gst_dvprodecoder_get_type())
#define GST_DVPRODECODER(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DVPRODECODER,GstDvprodecoder))
#define GST_DVPRODECODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DVPRODECODER,GstDvprodecoderClass))
#define GST_IS_DVPRODECODER(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DVPRODECODER))
#define GST_IS_DVPRODECODER_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DVPRODECODER))

typedef struct _GstDvprodecoder GstDvprodecoder;
typedef struct _GstDvprodecoderClass GstDvprodecoderClass;

struct _GstDvprodecoder
{
  GstVideoDecoder base_dvprodecoder;
  dvpd_handle ctx;
  dvpd_config_t cfg;
  gchar hevc_plugin_name[1024];
  dvpd_output_mode_t output_mode;
};

struct _GstDvprodecoderClass
{
  GstVideoDecoderClass base_dvprodecoder_class;
};

GType gst_dvprodecoder_get_type (void);

G_END_DECLS

#endif
