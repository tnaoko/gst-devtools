/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *  Author: Thiago Sousa Santos <thiago.sousa.santos@collabora.com>
 *
 * gst-validate-media-check.c - Media Check CLI tool
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <gst/gst.h>
#include <gst/validate/validate.h>
#include <gst/validate/media-descriptor-writer.h>
#include <gst/validate/media-descriptor-parser.h>
#include <gst/validate/media-descriptor.h>
#include <gst/pbutils/encoding-profile.h>

/* move this into some utils file */
#if 0
static gboolean
_parse_encoding_profile (const gchar * option_name, const gchar * value,
    gpointer udata, GError ** error)
{
  GstCaps *caps;
  char *preset_name = NULL;
  gchar **restriction_format, **preset_v;

  guint i, presence = 0;
  GstCaps *restrictioncaps = NULL;
  gchar **strpresence_v, **strcaps_v = g_strsplit (value, ":", 0);

  if (strcaps_v[0] && *strcaps_v[0]) {
    caps = gst_caps_from_string (strcaps_v[0]);
    if (caps == NULL) {
      g_printerr ("Could not parse caps %s", strcaps_v[0]);
      return FALSE;
    }
    encoding_profile =
        GST_ENCODING_PROFILE (gst_encoding_container_profile_new
        ("User profile", "User profile", caps, NULL));
    gst_caps_unref (caps);
  } else {
    encoding_profile = NULL;
  }

  for (i = 1; strcaps_v[i]; i++) {
    GstEncodingProfile *profile = NULL;
    gchar *strcaps, *strpresence;

    restriction_format = g_strsplit (strcaps_v[i], "->", 0);
    if (restriction_format[1]) {
      restrictioncaps = gst_caps_from_string (restriction_format[0]);
      strcaps = g_strdup (restriction_format[1]);
    } else {
      restrictioncaps = NULL;
      strcaps = g_strdup (restriction_format[0]);
    }
    g_strfreev (restriction_format);

    preset_v = g_strsplit (strcaps, "+", 0);
    if (preset_v[1]) {
      strpresence = preset_v[1];
      g_free (strcaps);
      strcaps = g_strdup (preset_v[0]);
    } else {
      strpresence = preset_v[0];
    }

    strpresence_v = g_strsplit (strpresence, "|", 0);
    if (strpresence_v[1]) {     /* We have a presence */
      gchar *endptr;

      if (preset_v[1]) {        /* We have preset and presence */
        preset_name = g_strdup (strpresence_v[0]);
      } else {                  /* We have a presence but no preset */
        g_free (strcaps);
        strcaps = g_strdup (strpresence_v[0]);
      }

      presence = strtoll (strpresence_v[1], &endptr, 10);
      if (endptr == strpresence_v[1]) {
        g_printerr ("Wrong presence %s\n", strpresence_v[1]);

        return FALSE;
      }
    } else {                    /* We have no presence */
      if (preset_v[1]) {        /* Not presence but preset */
        preset_name = g_strdup (preset_v[1]);
        g_free (strcaps);
        strcaps = g_strdup (preset_v[0]);
      }                         /* Else we have no presence nor preset */
    }
    g_strfreev (strpresence_v);
    g_strfreev (preset_v);

    GST_DEBUG ("Creating preset with restrictions: %" GST_PTR_FORMAT
        ", caps: %s, preset %s, presence %d", restrictioncaps, strcaps,
        preset_name ? preset_name : "none", presence);

    caps = gst_caps_from_string (strcaps);
    g_free (strcaps);
    if (caps == NULL) {
      g_warning ("Could not create caps for %s", strcaps_v[i]);

      return FALSE;
    }

    if (g_str_has_prefix (strcaps_v[i], "audio/")) {
      profile = GST_ENCODING_PROFILE (gst_encoding_audio_profile_new (caps,
              preset_name, restrictioncaps, presence));
    } else if (g_str_has_prefix (strcaps_v[i], "video/") ||
        g_str_has_prefix (strcaps_v[i], "image/")) {
      profile = GST_ENCODING_PROFILE (gst_encoding_video_profile_new (caps,
              preset_name, restrictioncaps, presence));
    }

    g_free (preset_name);
    gst_caps_unref (caps);
    if (restrictioncaps)
      gst_caps_unref (restrictioncaps);

    if (profile == NULL) {
      g_warning ("No way to create a preset for caps: %s", strcaps_v[i]);

      return FALSE;
    }

    if (encoding_profile) {
      if (gst_encoding_container_profile_add_profile
          (GST_ENCODING_CONTAINER_PROFILE (encoding_profile),
              profile) == FALSE) {
        g_warning ("Can not create a preset for caps: %s", strcaps_v[i]);

        return FALSE;
      }
    } else {
      encoding_profile = profile;
    }
  }
  g_strfreev (strcaps_v);

  return TRUE;
}
#endif

int
main (int argc, gchar ** argv)
{
  GOptionContext *ctx;

  GError *err = NULL;
  guint ret = 0;
  gchar *output_file = NULL;
  gchar *expected_file = NULL;
  gchar *output = NULL;
  GstMediaDescriptorWriter *writer;
  GstValidateRunner *runner;
  GstMediaDescriptorParser * reference = NULL;

  GOptionEntry options[] = {
    {"output-file", 'o', 0, G_OPTION_ARG_FILENAME,
          &output_file, "The output file to store the results",
        NULL},
    {"expected-results", 'e', 0, G_OPTION_ARG_FILENAME,
          &expected_file, "Path to file containing the expected results "
          "(or the last results found) for comparison with new results",
        NULL},
    {NULL}
  };

  g_set_prgname ("gst-validate-media-check-" GST_API_VERSION);
  ctx = g_option_context_new ("[URI]");
  g_option_context_set_summary (ctx, "Analizes a media file and writes "
      "the results to stdout or a file. Can also compare the results found "
      "with another results file for identifying regressions. The monitoring"
      " lib from gst-validate will be enabled during the tests to identify "
      "issues with the gstreamer elements involved with the media file's "
      "container and codec types");
  g_option_context_add_main_entries (ctx, options, NULL);

  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_printerr ("Error initializing: %s\n", err->message);
    g_option_context_free (ctx);
    exit (1);
  }

  gst_init (&argc, &argv);
  gst_validate_init ();

  if (argc != 2) {
    gchar *msg = g_option_context_get_help (ctx, TRUE, NULL);
    g_printerr ("%s\n", msg);
    g_free (msg);
    g_option_context_free (ctx);
    return 1;
  }
  g_option_context_free (ctx);

  runner = gst_validate_runner_new ();
  writer = gst_media_descriptor_writer_new_discover (runner, argv[1], NULL);
  if (writer == NULL) {
    g_print ("Could not discover file: %s", argv[1]);
    return 1;
  }

  if (output_file)
    gst_media_descriptor_writer_write (writer, output_file);

  if (expected_file) {
    reference = gst_media_descriptor_parser_new (runner,
        expected_file, NULL);

    if (reference == NULL) {
      g_print ("Could not parse file: %s", expected_file);
      gst_object_unref (writer);

      return 1;
    }

    gst_media_descriptors_compare (GST_MEDIA_DESCRIPTOR (reference),
        GST_MEDIA_DESCRIPTOR (writer));
  } else {
    output = gst_media_descriptor_writer_serialize (writer);
    g_print ("Media info:\n%s\n", output);
    g_free (output);
  }

  ret = gst_validate_runner_printf (runner);
  if (ret && expected_file) {
    output = gst_media_descriptor_writer_serialize (writer);
    g_print ("Media info:\n%s\n", output);
    g_free (output);
  }

  if (reference)
    gst_object_unref (reference);
  gst_object_unref (writer);
  gst_object_unref (runner);

  return ret;
}
