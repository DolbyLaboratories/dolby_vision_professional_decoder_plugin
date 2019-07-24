#include <gtest/gtest.h>
#include <gst/gst.h>

class GstDvProDecoderTest : public ::testing::Test
{
protected:
	GstElement *pipeline;
	GstBus *bus;
	GMainLoop *loop;
	gint count_eos;
	gint count_err;
	gint count_frames;

	GstDvProDecoderTest()
	{
		gst_init(NULL, NULL);
	}

	void SetUp() override {
		loop = g_main_loop_new(NULL, FALSE);
		ASSERT_NE(loop, nullptr);

		pipeline = nullptr;
		bus = nullptr;
		count_eos = 0;
		count_err = 0;
		count_frames = 0;
	}

	void TearDown() override {
		gst_element_set_state(pipeline, GST_STATE_NULL);

		gst_object_unref(bus);
		gst_object_unref(pipeline);

		g_main_loop_unref(loop);
	}

	void SetPipeline(std::string pipeline_string)
	{
		GError *err = nullptr;

		ASSERT_EQ(pipeline, nullptr);

		pipeline = gst_parse_launch(pipeline_string.c_str(), &err);
		ASSERT_EQ(err, nullptr);

		bus = gst_element_get_bus(pipeline);
		ASSERT_NE(bus, nullptr);

		gst_bus_add_signal_watch(bus);
		g_signal_connect(bus, "message", G_CALLBACK(BusMessages), this);

		GstIterator *it = gst_bin_iterate_elements(GST_BIN(pipeline));
		ASSERT_NE(it, nullptr);

		GValue vElement = G_VALUE_INIT;
		g_value_init(&vElement, GST_TYPE_ELEMENT);

		GstElementFactory *fakesink_factory = gst_element_factory_find("fakesink");
		GstElementFactory *dvprodecoder_factory = gst_element_factory_find("dvprodecoder");

		while (gst_iterator_next(it, &vElement) == GST_ITERATOR_OK)
		{
			GstElement *sink = GST_ELEMENT(g_value_get_object(&vElement));

			if (gst_element_get_factory(sink) == fakesink_factory)
			{
				g_object_set(sink, "signal-handoffs", TRUE, NULL);
				g_signal_connect(sink, "handoff", G_CALLBACK(FakeSinkHandOff), this);
			}
			else if (gst_element_get_factory(sink) == dvprodecoder_factory)
			{
			//	g_object_set(sink, "hevc-plugin", "libFFmpegHevcDecPlugin.so", NULL);
			}
			g_value_reset(&vElement);
		}
		gst_iterator_free(it);

		g_object_unref(fakesink_factory);
		g_object_unref(dvprodecoder_factory);
	}

	static gboolean TimeoutHit(gpointer user_data)
	{
		GstDvProDecoderTest *_this = (GstDvProDecoderTest*)user_data;

		g_main_loop_quit(_this->loop);

		return FALSE;
	}

	void Run(guint timeout = 0)
	{
		gst_element_set_state(pipeline, GST_STATE_PLAYING);

		if (timeout != 0)
		{
			GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ASYNC_DONE);
			gst_message_unref(msg);

			g_timeout_add(timeout, TimeoutHit, this);
		}

		g_main_loop_run(loop);
	}

	static gboolean BusMessages(GstBus *bus, GstMessage *message, void *user_data)
	{
		GstDvProDecoderTest *_this = (GstDvProDecoderTest*)user_data;

		switch (GST_MESSAGE_TYPE(message))
		{
			case GST_MESSAGE_EOS:
				_this->count_eos++;
				g_main_loop_quit(_this->loop);
				break;
			case GST_MESSAGE_ERROR:
				_this->count_err++;
				break;
			default:
				break;
		}
		return TRUE;
	}

	static void FakeSinkHandOff(GstElement* object, GstBuffer* arg0, GstPad* arg1, gpointer user_data)
	{
		GstDvProDecoderTest *_this = (GstDvProDecoderTest*)user_data;

		static GMutex mutex;

		g_mutex_lock(&mutex);
		_this->count_frames++;
		g_mutex_unlock(&mutex);
	}

};

TEST_F(GstDvProDecoderTest, GStreamer)
{
	SetPipeline("fakesrc num-buffers=1 ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 1);
}

TEST_F(GstDvProDecoderTest, PluginLoading)
{
	SetPipeline("fakesrc ! dvprodecoder ! fakesink");

	EXPECT_EQ(count_eos, 0);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 0);
}

TEST_F(GstDvProDecoderTest, RunOnce)
{
	SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18);
}

TEST_F(GstDvProDecoderTest, Restart)
{
	SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder ! fakesink");

	Run();

	gst_element_set_state(pipeline, GST_STATE_READY);

	Run();

	EXPECT_EQ(count_eos, 2);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18*count_eos);
}

TEST_F(GstDvProDecoderTest, Loop3Times)
{
	SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder ! fakesink");

	for (int i = 0; i < 3; i++)
	{
		Run();

		gst_element_seek_simple(pipeline, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);
	}

	EXPECT_EQ(count_eos, 3);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18*count_eos);
}

TEST_F(GstDvProDecoderTest, StopWhileRunning)
{
	SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder ! fakesink");

	Run(1000);

	gst_element_set_state(pipeline, GST_STATE_READY);

	EXPECT_EQ(count_eos, 0);
	EXPECT_EQ(count_err, 0);
	EXPECT_LT(count_frames, 18);
	EXPECT_GT(count_frames, 0);
}

TEST_F(GstDvProDecoderTest, SeekWhileRunning)
{
	SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder ! fakesink");

	Run(1000);

	gst_element_seek_simple(pipeline, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_GT(count_frames, 18);
}

TEST_F(GstDvProDecoderTest, MultipleInstances)
{
	SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
		! h265parse ! tee name=tee ! queue ! dvprodecoder ! fakesink tee. ! queue ! dvprodecoder ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18*2);
}

TEST_F(GstDvProDecoderTest, Profile_4)
{
	SetPipeline("filesrc location=data/dvhe_04_09_mel_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder input-mode=profile4 ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18);
}

TEST_F(GstDvProDecoderTest, Profile_8_1)
{
	SetPipeline("filesrc location=data/dvhe_08_1_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder input-mode=profile8 ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18);
}

TEST_F(GstDvProDecoderTest, Profile_8_1_Compression)
{
	SetPipeline("filesrc location=data/dvhe_08_1_09_rpu_compression_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder input-mode=profile8 ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18);
}

TEST_F(GstDvProDecoderTest, Profile_8_2)
{
	SetPipeline("filesrc location=data/dvhe_08_2_09_3840x2160_60fps.265 \
		! h265parse ! dvprodecoder input-mode=profile8 ! fakesink");

	Run();

	EXPECT_EQ(count_eos, 1);
	EXPECT_EQ(count_err, 0);
	EXPECT_EQ(count_frames, 18);
}

TEST_F(GstDvProDecoderTest, OutputModes)
{
	const int num_modes = 19;

	for (int i = 0; i < num_modes; i++)
	{
		SetPipeline("filesrc location=data/dvhe_05_09_3840x2160_60fps.265 \
			! h265parse ! dvprodecoder output-mode=" + std::to_string(i) + " ! fakesink");

		Run();

		EXPECT_EQ(count_eos, 1);
		EXPECT_EQ(count_err, 0);
		EXPECT_EQ(count_frames, 18);

		if (i != num_modes - 1)
		{
			TearDown();
			SetUp();
		}
	}
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
