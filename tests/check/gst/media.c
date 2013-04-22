/* GStreamer
 * Copyright (C) 2012 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/check/gstcheck.h>

#include <rtsp-media-factory.h>

GST_START_TEST (test_launch)
{
  GstRTSPMediaFactory *factory;
  GstRTSPMedia *media;
  GstRTSPUrl *url;
  GstRTSPStream *stream;
  GstRTSPTimeRange *range;
  gchar *str;

  factory = gst_rtsp_media_factory_new ();
  fail_if (gst_rtsp_media_factory_is_shared (factory));
  gst_rtsp_url_parse ("rtsp://localhost:8554/test", &url);

  gst_rtsp_media_factory_set_launch (factory,
      "( videotestsrc ! rtpvrawpay pt=96 name=pay0 )");

  media = gst_rtsp_media_factory_construct (factory, url);
  fail_unless (GST_IS_RTSP_MEDIA (media));

  fail_unless (gst_rtsp_media_n_streams (media) == 1);

  stream = gst_rtsp_media_get_stream (media, 0);
  fail_unless (stream != NULL);

  /* fails, need to be prepared */
  str = gst_rtsp_media_get_range_string (media, FALSE, GST_RTSP_RANGE_NPT);
  fail_unless (str == NULL);

  fail_unless (gst_rtsp_range_parse ("npt=5.0-", &range) == GST_RTSP_OK);
  /* fails, need to be prepared */
  fail_if (gst_rtsp_media_seek (media, range));

  fail_unless (gst_rtsp_media_prepare (media));

  str = gst_rtsp_media_get_range_string (media, FALSE, GST_RTSP_RANGE_NPT);
  fail_unless (g_str_equal (str, "npt=0-"));
  g_free (str);

  str = gst_rtsp_media_get_range_string (media, TRUE, GST_RTSP_RANGE_NPT);
  fail_unless (g_str_equal (str, "npt=0-"));
  g_free (str);

  fail_unless (gst_rtsp_media_seek (media, range));

  str = gst_rtsp_media_get_range_string (media, FALSE, GST_RTSP_RANGE_NPT);
  fail_unless (g_str_equal (str, "npt=5-"));
  g_free (str);

  str = gst_rtsp_media_get_range_string (media, TRUE, GST_RTSP_RANGE_NPT);
  fail_unless (g_str_equal (str, "npt=5-"));
  g_free (str);

  fail_unless (gst_rtsp_media_unprepare (media));

  /* should fail again */
  str = gst_rtsp_media_get_range_string (media, FALSE, GST_RTSP_RANGE_NPT);
  fail_unless (str == NULL);
  fail_if (gst_rtsp_media_seek (media, range));

  gst_rtsp_range_free (range);
  g_object_unref (media);

  gst_rtsp_url_free (url);
  g_object_unref (factory);
}

GST_END_TEST;

GST_START_TEST (test_media)
{
  GstRTSPMedia *media;
  GstElement *bin, *e1, *e2;

  bin = gst_bin_new ("bin");
  fail_if (bin == NULL);

  e1 = gst_element_factory_make ("videotestsrc", NULL);
  fail_if (e1 == NULL);

  e2 = gst_element_factory_make ("rtpvrawpay", "pay0");
  fail_if (e2 == NULL);
  g_object_set (e2, "pt", 96, NULL);

  gst_bin_add_many (GST_BIN_CAST (bin), e1, e2, NULL);
  gst_element_link_many (e1, e2, NULL);

  media = gst_rtsp_media_new (bin);
  fail_unless (GST_IS_RTSP_MEDIA (media));
  g_object_unref (media);
}

GST_END_TEST;

GST_START_TEST (test_media_prepare)
{
  GstRTSPMediaFactory *factory;
  GstRTSPMedia *media;
  GstRTSPUrl *url;

  /* test non-reusable media first */
  factory = gst_rtsp_media_factory_new ();
  fail_if (gst_rtsp_media_factory_is_shared (factory));
  gst_rtsp_url_parse ("rtsp://localhost:8554/test", &url);

  gst_rtsp_media_factory_set_launch (factory,
      "( videotestsrc ! rtpvrawpay pt=96 name=pay0 )");

  media = gst_rtsp_media_factory_construct (factory, url);
  fail_unless (GST_IS_RTSP_MEDIA (media));
  fail_unless (gst_rtsp_media_n_streams (media) == 1);

  fail_unless (gst_rtsp_media_prepare (media));
  fail_unless (gst_rtsp_media_unprepare (media));
  fail_unless (gst_rtsp_media_n_streams (media) == 1);
  fail_if (gst_rtsp_media_prepare (media));

  g_object_unref (media);
  gst_rtsp_url_free (url);
  g_object_unref (factory);

  /* test reusable media */
  factory = gst_rtsp_media_factory_new ();
  fail_if (gst_rtsp_media_factory_is_shared (factory));
  gst_rtsp_url_parse ("rtsp://localhost:8554/test", &url);

  gst_rtsp_media_factory_set_launch (factory,
      "( videotestsrc ! rtpvrawpay pt=96 name=pay0 )");

  media = gst_rtsp_media_factory_construct (factory, url);
  fail_unless (GST_IS_RTSP_MEDIA (media));
  fail_unless (gst_rtsp_media_n_streams (media) == 1);

  g_object_set (G_OBJECT (media), "reusable", TRUE, NULL);

  fail_unless (gst_rtsp_media_prepare (media));
  fail_unless (gst_rtsp_media_unprepare (media));
  fail_unless (gst_rtsp_media_n_streams (media) == 1);
  fail_unless (gst_rtsp_media_prepare (media));
  fail_unless (gst_rtsp_media_unprepare (media));

  g_object_unref (media);
  gst_rtsp_url_free (url);
  g_object_unref (factory);
}

GST_END_TEST;

static Suite *
rtspmedia_suite (void)
{
  Suite *s = suite_create ("rtspmedia");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (s, tc);
  tcase_set_timeout (tc, 20);
  tcase_add_test (tc, test_launch);
  tcase_add_test (tc, test_media);
  tcase_add_test (tc, test_media_prepare);

  return s;
}

GST_CHECK_MAIN (rtspmedia);