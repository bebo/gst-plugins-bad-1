/* GStreamer
 * Copyright (C) 2008-2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <string.h>

#include "mxftypes.h"

GST_DEBUG_CATEGORY_EXTERN (mxf_debug);
#define GST_CAT_DEFAULT mxf_debug

gboolean
mxf_is_mxf_packet (const MXFUL * ul)
{
  return mxf_ul_is_subclass (MXF_UL (SMPTE), ul);
}

/* SMPTE 377M 6.1: Check if this is a valid partition pack */
gboolean
mxf_is_partition_pack (const MXFUL * ul)
{
  if (mxf_ul_is_subclass (MXF_UL (PARTITION_PACK), ul) &&
      ul->u[13] >= 0x02 && ul->u[13] <= 0x04 &&
      ul->u[14] < 0x05 && ul->u[15] == 0x00)
    return TRUE;

  return FALSE;
}

/* SMPTE 377M 6.2: header partition pack has byte 14 == 0x02 */
gboolean
mxf_is_header_partition_pack (const MXFUL * ul)
{
  if (mxf_is_partition_pack (ul) && ul->u[13] == 0x02)
    return TRUE;

  return FALSE;
}

/* SMPTE 377M 6.3: body partition pack has byte 14 == 0x03 */
gboolean
mxf_is_body_partition_pack (const MXFUL * ul)
{
  if (mxf_is_partition_pack (ul) && ul->u[13] == 0x03)
    return TRUE;

  return FALSE;
}

/* SMPTE 377M 6.4: footer partition pack has byte 14 == 0x04 */
gboolean
mxf_is_footer_partition_pack (const MXFUL * ul)
{
  if (mxf_is_partition_pack (ul) && ul->u[13] == 0x04)
    return TRUE;

  return FALSE;
}

gboolean
mxf_is_fill (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (FILL), ul));
}

gboolean
mxf_is_primer_pack (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (PRIMER_PACK), ul));
}

gboolean
mxf_is_metadata (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (METADATA), ul));
}

/* SMPTE 377M 8.7.3 */
gboolean
mxf_is_descriptive_metadata (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (DESCRIPTIVE_METADATA), ul));
}

gboolean
mxf_is_random_index_pack (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (RANDOM_INDEX_PACK), ul));
}

gboolean
mxf_is_index_table_segment (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (INDEX_TABLE_SEGMENT), ul));
}

/* SMPTE 379M 6.2.1 */
gboolean
mxf_is_generic_container_system_item (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (GENERIC_CONTAINER_SYSTEM_ITEM), ul) &&
      (ul->u[12] == 0x04 || ul->u[12] == 0x14));
}

/* SMPTE 379M 7.1 */
gboolean
mxf_is_generic_container_essence_element (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (GENERIC_CONTAINER_ESSENCE_ELEMENT), ul)
      && (ul->u[12] == 0x05 || ul->u[12] == 0x06
          || ul->u[12] == 0x07 || ul->u[12] == 0x15
          || ul->u[12] == 0x16 || ul->u[12] == 0x17 || ul->u[12] == 0x18));
}

/* SMPTE 379M 8 */
gboolean
mxf_is_generic_container_essence_container_label (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL
          (GENERIC_CONTAINER_ESSENCE_CONTAINER_LABEL), ul) && (ul->u[12] == 0x01
          || ul->u[12] == 0x02));
}

/* Essence container label found in files generated by Avid */
gboolean
mxf_is_avid_essence_container_label (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (AVID_ESSENCE_CONTAINER_ESSENCE_LABEL),
          ul));
}

/* Essence element key found in files generated by Avid */
gboolean
mxf_is_avid_essence_container_essence_element (const MXFUL * ul)
{
  return (mxf_ul_is_subclass (MXF_UL (AVID_ESSENCE_CONTAINER_ESSENCE_ELEMENT),
          ul));
}

guint
mxf_ber_encode_size (guint size, guint8 ber[9])
{
  guint8 slen, i;
  guint8 tmp[8];

  memset (ber, 0, 9);

  if (size <= 127) {
    ber[0] = size;
    return 1;
  }

  slen = 0;
  while (size > 0) {
    tmp[slen] = size & 0xff;
    size >>= 8;
    slen++;
  }

  ber[0] = 0x80 | slen;
  for (i = 0; i < slen; i++) {
    ber[i + 1] = tmp[slen - i - 1];
  }

  return slen + 1;
}

GstBuffer *
mxf_fill_to_buffer (guint size)
{
  GstBuffer *ret;
  GstMapInfo map;
  guint slen;
  guint8 ber[9];

  slen = mxf_ber_encode_size (size, ber);

  ret = gst_buffer_new_and_alloc (16 + slen + size);
  gst_buffer_map (ret, &map, GST_MAP_WRITE);

  memcpy (map.data, MXF_UL (FILL), 16);
  memcpy (map.data + 16, &ber, slen);
  memset (map.data + slen, 0, size);

  gst_buffer_unmap (ret, &map);

  return ret;
}

void
mxf_uuid_init (MXFUUID * uuid, GHashTable * hashtable)
{
  guint i;

  do {
    for (i = 0; i < 4; i++)
      GST_WRITE_UINT32_BE (&uuid->u[i * 4], g_random_int ());

  } while (hashtable && (mxf_uuid_is_zero (uuid) ||
          g_hash_table_lookup_extended (hashtable, uuid, NULL, NULL)));
}

gboolean
mxf_uuid_is_equal (const MXFUUID * a, const MXFUUID * b)
{
  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);

  return (memcmp (a, b, 16) == 0);
}

gboolean
mxf_uuid_is_zero (const MXFUUID * a)
{
  static const guint8 zero[16] = { 0x00, };

  g_return_val_if_fail (a != NULL, FALSE);

  return (memcmp (a, zero, 16) == 0);
}

guint
mxf_uuid_hash (const MXFUUID * uuid)
{
  guint32 ret = 0;
  guint i;

  g_return_val_if_fail (uuid != NULL, 0);

  for (i = 0; i < 4; i++)
    ret ^= (uuid->u[i * 4 + 0] << 24) |
        (uuid->u[i * 4 + 1] << 16) |
        (uuid->u[i * 4 + 2] << 8) | (uuid->u[i * 4 + 3] << 0);

  return ret;
}

gchar *
mxf_uuid_to_string (const MXFUUID * uuid, gchar str[48])
{
  gchar *ret = str;

  g_return_val_if_fail (uuid != NULL, NULL);

  if (ret == NULL)
    ret = g_malloc (48);

  g_snprintf (ret, 48,
      "%02x.%02x.%02x.%02x."
      "%02x.%02x.%02x.%02x."
      "%02x.%02x.%02x.%02x."
      "%02x.%02x.%02x.%02x",
      uuid->u[0], uuid->u[1], uuid->u[2], uuid->u[3],
      uuid->u[4], uuid->u[5], uuid->u[6], uuid->u[7],
      uuid->u[8], uuid->u[9], uuid->u[10], uuid->u[11],
      uuid->u[12], uuid->u[13], uuid->u[14], uuid->u[15]);

  return ret;
}

MXFUUID *
mxf_uuid_from_string (const gchar * str, MXFUUID * uuid)
{
  MXFUUID *ret = uuid;
  gint len;
  guint i, j;

  g_return_val_if_fail (str != NULL, NULL);

  len = strlen (str);
  if (len != 47) {
    GST_ERROR ("Invalid UUID string length %d, should be 47", len);
    return NULL;
  }

  if (ret == NULL)
    ret = g_new0 (MXFUUID, 1);

  memset (ret, 0, 16);

  for (i = 0, j = 0; i < 16; i++) {
    if (!g_ascii_isxdigit (str[j]) ||
        !g_ascii_isxdigit (str[j + 1]) ||
        (str[j + 2] != '.' && str[j + 2] != '\0')) {
      GST_ERROR ("Invalid UL string '%s'", str);
      if (uuid == NULL)
        g_free (ret);
      return NULL;
    }

    ret->u[i] = (g_ascii_xdigit_value (str[j]) << 4) |
        (g_ascii_xdigit_value (str[j + 1]));
    j += 3;
  }
  return ret;
}

gboolean
mxf_uuid_array_parse (MXFUUID ** array, guint32 * count, const guint8 * data,
    guint size)
{
  guint32 element_count, element_size;
  guint i;

  g_return_val_if_fail (array != NULL, FALSE);
  g_return_val_if_fail (count != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  if (size < 8)
    return FALSE;

  element_count = GST_READ_UINT32_BE (data);
  data += 4;
  size -= 4;

  if (element_count == 0) {
    *array = NULL;
    *count = 0;
    return TRUE;
  }

  element_size = GST_READ_UINT32_BE (data);
  data += 4;
  size -= 4;

  if (element_size != 16) {
    *array = NULL;
    *count = 0;
    return FALSE;
  }

  if (16 * element_count < size) {
    *array = NULL;
    *count = 0;
    return FALSE;
  }

  *array = g_new (MXFUUID, element_count);
  *count = element_count;

  for (i = 0; i < element_count; i++) {
    memcpy (&((*array)[i]), data, 16);
    data += 16;
  }

  return TRUE;
}

gboolean
mxf_umid_is_equal (const MXFUMID * a, const MXFUMID * b)
{
  return (memcmp (a, b, 32) == 0);
}

gboolean
mxf_umid_is_zero (const MXFUMID * umid)
{
  static const MXFUMID zero = { {0,} };

  return (memcmp (umid, &zero, 32) == 0);
}

gchar *
mxf_umid_to_string (const MXFUMID * umid, gchar str[96])
{
  g_return_val_if_fail (umid != NULL, NULL);
  g_return_val_if_fail (str != NULL, NULL);

  g_snprintf (str, 96,
      "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x."
      "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
      umid->u[0], umid->u[1], umid->u[2], umid->u[3], umid->u[4], umid->u[5],
      umid->u[6], umid->u[7], umid->u[8], umid->u[9], umid->u[10], umid->u[11],
      umid->u[12], umid->u[13], umid->u[14], umid->u[15],
      umid->u[16],
      umid->u[17],
      umid->u[18],
      umid->u[19],
      umid->u[20],
      umid->u[21],
      umid->u[22],
      umid->u[23],
      umid->u[24],
      umid->u[25],
      umid->u[26], umid->u[27], umid->u[28], umid->u[29], umid->u[30],
      umid->u[31]
      );

  return str;
}

MXFUMID *
mxf_umid_from_string (const gchar * str, MXFUMID * umid)
{
  gint len;
  guint i, j;

  g_return_val_if_fail (str != NULL, NULL);
  len = strlen (str);

  memset (umid, 0, 32);

  if (len != 95) {
    GST_ERROR ("Invalid UMID string length %d", len);
    return NULL;
  }

  for (i = 0, j = 0; i < 32; i++) {
    if (!g_ascii_isxdigit (str[j]) ||
        !g_ascii_isxdigit (str[j + 1]) ||
        (str[j + 2] != '.' && str[j + 2] != '\0')) {
      GST_ERROR ("Invalid UMID string '%s'", str);
      return NULL;
    }

    umid->u[i] =
        (g_ascii_xdigit_value (str[j]) << 4) | (g_ascii_xdigit_value (str[j +
                1]));
    j += 3;
  }
  return umid;
}

void
mxf_umid_init (MXFUMID * umid)
{
  guint i;
  guint32 tmp;

  /* SMPTE S330M 5.1.1:
   *    UMID Identifier
   */
  umid->u[0] = 0x06;
  umid->u[1] = 0x0a;
  umid->u[2] = 0x2b;
  umid->u[3] = 0x34;
  umid->u[4] = 0x01;
  umid->u[5] = 0x01;
  umid->u[6] = 0x01;
  umid->u[7] = 0x05;            /* version, see RP210 */
  umid->u[8] = 0x01;
  umid->u[9] = 0x01;
  umid->u[10] = 0x0d;           /* mixed group of components in a single container */

  /* - UUID/UL method for material number
   * - 24 bit PRG for instance number
   */
  umid->u[11] = 0x20 | 0x02;

  /* Length of remaining data */
  umid->u[12] = 0x13;

  /* Instance number */
  tmp = g_random_int ();
  umid->u[13] = (tmp >> 24) & 0xff;
  umid->u[14] = (tmp >> 16) & 0xff;
  umid->u[15] = (tmp >> 8) & 0xff;

  /* Material number: ISO UUID Version 4 */
  for (i = 16; i < 32; i += 4)
    GST_WRITE_UINT32_BE (&umid->u[i], g_random_int ());

  umid->u[16 + 6] &= 0x0f;
  umid->u[16 + 6] |= 0x40;

  umid->u[16 + 8] &= 0x3f;
  umid->u[16 + 8] |= 0x80;
}

gboolean
mxf_timestamp_parse (MXFTimestamp * timestamp, const guint8 * data, guint size)
{
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (timestamp != NULL, FALSE);

  memset (timestamp, 0, sizeof (MXFTimestamp));

  if (size < 8)
    return FALSE;

  timestamp->year = GST_READ_UINT16_BE (data);
  timestamp->month = GST_READ_UINT8 (data + 2);
  timestamp->day = GST_READ_UINT8 (data + 3);
  timestamp->hour = GST_READ_UINT8 (data + 4);
  timestamp->minute = GST_READ_UINT8 (data + 5);
  timestamp->second = GST_READ_UINT8 (data + 6);
  timestamp->msecond = (GST_READ_UINT8 (data + 7) * 1000) / 256;

  return TRUE;
}

/* SMPTE 377M 3.3: A value of 0 for every field means unknown timestamp */
gboolean
mxf_timestamp_is_unknown (const MXFTimestamp * a)
{
  static const MXFTimestamp unknown = { 0, 0, 0, 0, 0, 0, 0 };

  return (memcmp (a, &unknown, sizeof (MXFTimestamp)) == 0);
}

gint
mxf_timestamp_compare (const MXFTimestamp * a, const MXFTimestamp * b)
{
  gint diff;

  if ((diff = a->year - b->year) != 0)
    return diff;
  else if ((diff = a->month - b->month) != 0)
    return diff;
  else if ((diff = a->day - b->day) != 0)
    return diff;
  else if ((diff = a->hour - b->hour) != 0)
    return diff;
  else if ((diff = a->minute - b->minute) != 0)
    return diff;
  else if ((diff = a->second - b->second) != 0)
    return diff;
  else if ((diff = a->msecond - b->msecond) != 0)
    return diff;
  else
    return 0;
}

gchar *
mxf_timestamp_to_string (const MXFTimestamp * t, gchar str[32])
{
  g_snprintf (str, 32,
      "%04d-%02u-%02u %02u:%02u:%02u.%03u", t->year, t->month,
      t->day, t->hour, t->minute, t->second, t->msecond);
  return str;
}

void
mxf_timestamp_set_now (MXFTimestamp * timestamp)
{
  GTimeVal tv;
  time_t t;
  struct tm *tm;

#ifdef HAVE_GMTIME_R
  struct tm tm_;
#endif

  g_get_current_time (&tv);
  t = (time_t) tv.tv_sec;

#ifdef HAVE_GMTIME_R
  tm = gmtime_r (&t, &tm_);
#else
  tm = gmtime (&t);
#endif

  timestamp->year = tm->tm_year + 1900;
  timestamp->month = tm->tm_mon;
  timestamp->day = tm->tm_mday;
  timestamp->hour = tm->tm_hour;
  timestamp->minute = tm->tm_min;
  timestamp->second = tm->tm_sec;
  timestamp->msecond = tv.tv_usec / 1000;
}

void
mxf_timestamp_write (const MXFTimestamp * timestamp, guint8 * data)
{
  GST_WRITE_UINT16_BE (data, timestamp->year);
  GST_WRITE_UINT8 (data + 2, timestamp->month);
  GST_WRITE_UINT8 (data + 3, timestamp->day);
  GST_WRITE_UINT8 (data + 4, timestamp->hour);
  GST_WRITE_UINT8 (data + 5, timestamp->minute);
  GST_WRITE_UINT8 (data + 6, timestamp->second);
  GST_WRITE_UINT8 (data + 7, (timestamp->msecond * 256) / 1000);
}

gboolean
mxf_fraction_parse (MXFFraction * fraction, const guint8 * data, guint size)
{
  g_return_val_if_fail (fraction != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  memset (fraction, 0, sizeof (MXFFraction));

  if (size < 8)
    return FALSE;

  fraction->n = GST_READ_UINT32_BE (data);
  fraction->d = GST_READ_UINT32_BE (data + 4);

  return TRUE;
}

gdouble
mxf_fraction_to_double (const MXFFraction * fraction)
{
  return ((gdouble) fraction->n) / ((gdouble) fraction->d);
}

gchar *
mxf_utf16_to_utf8 (const guint8 * data, guint size)
{
  gchar *ret;
  GError *error = NULL;

  ret =
      g_convert ((const gchar *) data, size, "UTF-8", "UTF-16BE", NULL, NULL,
      &error);

  if (ret == NULL) {
    GST_WARNING ("UTF-16-BE to UTF-8 conversion failed: %s", error->message);
    g_error_free (error);
    return NULL;
  }

  return ret;
}

guint8 *
mxf_utf8_to_utf16 (const gchar * str, guint16 * size)
{
  guint8 *ret;
  GError *error = NULL;
  gsize s;

  g_return_val_if_fail (size != NULL, NULL);

  if (str == NULL) {
    *size = 0;
    return NULL;
  }

  ret = (guint8 *)
      g_convert_with_fallback (str, -1, "UTF-16BE", "UTF-8", (char *) "*", NULL,
      &s, &error);

  if (ret == NULL) {
    GST_WARNING ("UTF-16-BE to UTF-8 conversion failed: %s", error->message);
    g_error_free (error);
    *size = 0;
    return NULL;
  }

  *size = s;
  return (guint8 *) ret;
}

gboolean
mxf_product_version_parse (MXFProductVersion * product_version,
    const guint8 * data, guint size)
{
  g_return_val_if_fail (product_version != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  memset (product_version, 0, sizeof (MXFProductVersion));

  if (size < 9)
    return FALSE;

  product_version->major = GST_READ_UINT16_BE (data);
  product_version->minor = GST_READ_UINT16_BE (data + 2);
  product_version->patch = GST_READ_UINT16_BE (data + 4);
  product_version->build = GST_READ_UINT16_BE (data + 6);

  /* Avid writes a 9 byte product version */
  if (size == 9)
    product_version->release = GST_READ_UINT8 (data + 8);
  else
    product_version->release = GST_READ_UINT16_BE (data + 8);

  return TRUE;
}

gboolean
mxf_product_version_is_valid (const MXFProductVersion * version)
{
  static const guint8 null[sizeof (MXFProductVersion)] = { 0, };

  return (memcmp (version, &null, sizeof (MXFProductVersion)) == 0);
}

void
mxf_product_version_write (const MXFProductVersion * version, guint8 * data)
{
  GST_WRITE_UINT16_BE (data, version->major);
  GST_WRITE_UINT16_BE (data + 2, version->minor);
  GST_WRITE_UINT16_BE (data + 4, version->patch);
  GST_WRITE_UINT16_BE (data + 6, version->build);
  GST_WRITE_UINT16_BE (data + 8, version->release);
}

void
mxf_op_set_atom (MXFUL * ul, gboolean single_sourceclip,
    gboolean single_essence_track)
{
  memcpy (&ul->u, MXF_UL (OPERATIONAL_PATTERN_IDENTIFICATION), 12);
  ul->u[12] = 0x10;
  ul->u[13] = 0;

  if (!single_sourceclip)
    ul->u[13] |= 0x80;

  if (!single_essence_track)
    ul->u[13] |= 0x40;

  ul->u[14] = 0;
  ul->u[15] = 0;
}

void
mxf_op_set_generalized (MXFUL * ul, MXFOperationalPattern pattern,
    gboolean internal_essence, gboolean streamable, gboolean single_track)
{
  g_return_if_fail (pattern >= MXF_OP_1a);

  memcpy (&ul->u, MXF_UL (OPERATIONAL_PATTERN_IDENTIFICATION), 12);

  if (pattern == MXF_OP_1a || pattern == MXF_OP_1b || pattern == MXF_OP_1c)
    ul->u[12] = 0x01;
  else if (pattern == MXF_OP_2a || pattern == MXF_OP_2b || pattern == MXF_OP_2c)
    ul->u[12] = 0x02;
  else if (pattern == MXF_OP_3a || pattern == MXF_OP_3b || pattern == MXF_OP_3c)
    ul->u[12] = 0x03;

  if (pattern == MXF_OP_1a || pattern == MXF_OP_2a || pattern == MXF_OP_3a)
    ul->u[13] = 0x01;
  else if (pattern == MXF_OP_1b || pattern == MXF_OP_2b || pattern == MXF_OP_3b)
    ul->u[13] = 0x02;
  else if (pattern == MXF_OP_1c || pattern == MXF_OP_2c || pattern == MXF_OP_3c)
    ul->u[13] = 0x02;

  ul->u[14] = 0x80;
  if (!internal_essence)
    ul->u[14] |= 0x40;
  if (!streamable)
    ul->u[14] |= 0x20;
  if (!single_track)
    ul->u[14] |= 0x10;

  ul->u[15] = 0;
}

/* SMPTE 377M 6.1, Table 2 */
gboolean
mxf_partition_pack_parse (const MXFUL * ul, MXFPartitionPack * pack,
    const guint8 * data, guint size)
{
#ifndef GST_DISABLE_GST_DEBUG
  guint i;
  gchar str[48];
#endif

  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (size >= 84, FALSE);

  memset (pack, 0, sizeof (MXFPartitionPack));

  GST_DEBUG ("Parsing partition pack:");

  if (ul->u[13] == 0x02)
    pack->type = MXF_PARTITION_PACK_HEADER;
  else if (ul->u[13] == 0x03)
    pack->type = MXF_PARTITION_PACK_BODY;
  else if (ul->u[13] == 0x04)
    pack->type = MXF_PARTITION_PACK_FOOTER;

  GST_DEBUG ("  type = %s",
      (pack->type == MXF_PARTITION_PACK_HEADER) ? "header" : (pack->type ==
          MXF_PARTITION_PACK_BODY) ? "body" : "footer");

  pack->closed = (ul->u[14] == 0x02 || ul->u[14] == 0x04);
  pack->complete = (ul->u[14] == 0x03 || ul->u[14] == 0x04);

  GST_DEBUG ("  closed = %s, complete = %s", (pack->closed) ? "yes" : "no",
      (pack->complete) ? "yes" : "no");

  pack->major_version = GST_READ_UINT16_BE (data);
  if (pack->major_version != 1)
    goto error;
  data += 2;
  size -= 2;

  pack->minor_version = GST_READ_UINT16_BE (data);
  data += 2;
  size -= 2;

  GST_DEBUG ("  MXF version = %u.%u", pack->major_version, pack->minor_version);

  pack->kag_size = GST_READ_UINT32_BE (data);
  data += 4;
  size -= 4;

  GST_DEBUG ("  KAG size = %u", pack->kag_size);

  pack->this_partition = GST_READ_UINT64_BE (data);
  data += 8;
  size -= 8;

  GST_DEBUG ("  this partition offset = %" G_GUINT64_FORMAT,
      pack->this_partition);

  pack->prev_partition = GST_READ_UINT64_BE (data);
  data += 8;
  size -= 8;

  GST_DEBUG ("  previous partition offset = %" G_GUINT64_FORMAT,
      pack->prev_partition);

  pack->footer_partition = GST_READ_UINT64_BE (data);
  data += 8;
  size -= 8;

  GST_DEBUG ("  footer partition offset = %" G_GUINT64_FORMAT,
      pack->footer_partition);

  pack->header_byte_count = GST_READ_UINT64_BE (data);
  data += 8;
  size -= 8;

  GST_DEBUG ("  header byte count = %" G_GUINT64_FORMAT,
      pack->header_byte_count);

  pack->index_byte_count = GST_READ_UINT64_BE (data);
  data += 8;
  size -= 8;

  pack->index_sid = GST_READ_UINT32_BE (data);
  data += 4;
  size -= 4;

  GST_DEBUG ("  index sid = %u, size = %" G_GUINT64_FORMAT, pack->index_sid,
      pack->index_byte_count);

  pack->body_offset = GST_READ_UINT64_BE (data);
  data += 8;
  size -= 8;

  pack->body_sid = GST_READ_UINT32_BE (data);
  data += 4;
  size -= 4;

  GST_DEBUG ("  body sid = %u, offset = %" G_GUINT64_FORMAT, pack->body_sid,
      pack->body_offset);

  memcpy (&pack->operational_pattern, data, 16);
  data += 16;
  size -= 16;

  GST_DEBUG ("  operational pattern = %s",
      mxf_ul_to_string (&pack->operational_pattern, str));

  if (!mxf_ul_array_parse (&pack->essence_containers,
          &pack->n_essence_containers, data, size))
    goto error;

#ifndef GST_DISABLE_GST_DEBUG
  GST_DEBUG ("  number of essence containers = %u", pack->n_essence_containers);
  if (pack->n_essence_containers) {
    for (i = 0; i < pack->n_essence_containers; i++) {
      GST_DEBUG ("  essence container %u = %s", i,
          mxf_ul_to_string (&pack->essence_containers[i], str));
    }
  }
#endif

  return TRUE;

error:
  GST_ERROR ("Invalid partition pack");

  mxf_partition_pack_reset (pack);
  return FALSE;
}

void
mxf_partition_pack_reset (MXFPartitionPack * pack)
{
  g_return_if_fail (pack != NULL);

  g_free (pack->essence_containers);

  memset (pack, 0, sizeof (MXFPartitionPack));
}

GstBuffer *
mxf_partition_pack_to_buffer (const MXFPartitionPack * pack)
{
  guint slen;
  guint8 ber[9];
  GstBuffer *ret;
  GstMapInfo map;
  guint8 *data;
  guint i;
  guint size =
      8 + 16 * pack->n_essence_containers + 16 + 4 + 8 + 4 + 8 + 8 + 8 + 8 + 8 +
      4 + 2 + 2;

  slen = mxf_ber_encode_size (size, ber);

  ret = gst_buffer_new_and_alloc (16 + slen + size);
  gst_buffer_map (ret, &map, GST_MAP_WRITE);

  memcpy (map.data, MXF_UL (PARTITION_PACK), 13);
  if (pack->type == MXF_PARTITION_PACK_HEADER)
    map.data[13] = 0x02;
  else if (pack->type == MXF_PARTITION_PACK_BODY)
    map.data[13] = 0x03;
  else if (pack->type == MXF_PARTITION_PACK_FOOTER)
    map.data[13] = 0x04;
  map.data[14] = 0;
  if (pack->complete)
    map.data[14] |= 0x02;
  if (pack->closed)
    map.data[14] |= 0x01;
  map.data[14] += 1;
  map.data[15] = 0;
  memcpy (map.data + 16, &ber, slen);

  data = map.data + 16 + slen;

  GST_WRITE_UINT16_BE (data, pack->major_version);
  GST_WRITE_UINT16_BE (data + 2, pack->minor_version);
  data += 4;

  GST_WRITE_UINT32_BE (data, pack->kag_size);
  data += 4;

  GST_WRITE_UINT64_BE (data, pack->this_partition);
  data += 8;

  GST_WRITE_UINT64_BE (data, pack->prev_partition);
  data += 8;

  GST_WRITE_UINT64_BE (data, pack->footer_partition);
  data += 8;

  GST_WRITE_UINT64_BE (data, pack->header_byte_count);
  data += 8;

  GST_WRITE_UINT64_BE (data, pack->index_byte_count);
  data += 8;

  GST_WRITE_UINT32_BE (data, pack->index_sid);
  data += 4;

  GST_WRITE_UINT64_BE (data, pack->body_offset);
  data += 8;

  GST_WRITE_UINT32_BE (data, pack->body_sid);
  data += 4;

  memcpy (data, &pack->operational_pattern, 16);
  data += 16;

  GST_WRITE_UINT32_BE (data, pack->n_essence_containers);
  GST_WRITE_UINT32_BE (data + 4, 16);
  data += 8;

  for (i = 0; i < pack->n_essence_containers; i++)
    memcpy (data + 16 * i, &pack->essence_containers[i], 16);

  gst_buffer_unmap (ret, &map);

  return ret;
}

/* SMPTE 377M 11.1 */
gboolean
mxf_random_index_pack_parse (const MXFUL * ul, const guint8 * data, guint size,
    GArray ** array)
{
  guint len, i;
  MXFRandomIndexPackEntry entry;

  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (array != NULL, FALSE);

  if (size < 4)
    return FALSE;

  if ((size - 4) % 12 != 0)
    return FALSE;

  GST_DEBUG ("Parsing random index pack:");

  len = (size - 4) / 12;

  GST_DEBUG ("  number of entries = %u", len);

  *array =
      g_array_sized_new (FALSE, FALSE, sizeof (MXFRandomIndexPackEntry), len);

  for (i = 0; i < len; i++) {
    entry.body_sid = GST_READ_UINT32_BE (data);
    entry.offset = GST_READ_UINT64_BE (data + 4);
    data += 12;

    GST_DEBUG ("  entry %u = body sid %u at offset %" G_GUINT64_FORMAT, i,
        entry.body_sid, entry.offset);

    g_array_append_val (*array, entry);
  }

  return TRUE;
}

GstBuffer *
mxf_random_index_pack_to_buffer (const GArray * array)
{
  MXFRandomIndexPackEntry *entry;
  guint i;
  GstBuffer *ret;
  GstMapInfo map;
  guint8 slen, ber[9];
  guint size;
  guint8 *data;

  if (array->len == 0)
    return NULL;

  size = array->len * 12 + 4;
  slen = mxf_ber_encode_size (size, ber);
  ret = gst_buffer_new_and_alloc (16 + slen + size);
  gst_buffer_map (ret, &map, GST_MAP_WRITE);

  memcpy (map.data, MXF_UL (RANDOM_INDEX_PACK), 16);
  memcpy (map.data + 16, ber, slen);

  data = map.data + 16 + slen;

  for (i = 0; i < array->len; i++) {
    entry = &g_array_index (array, MXFRandomIndexPackEntry, i);
    GST_WRITE_UINT32_BE (data, entry->body_sid);
    GST_WRITE_UINT64_BE (data + 4, entry->offset);
    data += 12;
  }
  GST_WRITE_UINT32_BE (data, gst_buffer_get_size (ret));

  gst_buffer_unmap (ret, &map);

  return ret;
}

/* SMPTE 377M 10.2.3 */
gboolean
mxf_index_table_segment_parse (const MXFUL * ul,
    MXFIndexTableSegment * segment, const MXFPrimerPack * primer,
    const guint8 * data, guint size)
{
#ifndef GST_DISABLE_GST_DEBUG
  gchar str[48];
#endif
  guint16 tag, tag_size;
  const guint8 *tag_data;

  g_return_val_if_fail (ul != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (primer != NULL, FALSE);

  memset (segment, 0, sizeof (MXFIndexTableSegment));

  if (size < 70)
    return FALSE;

  GST_DEBUG ("Parsing index table segment:");

  while (mxf_local_tag_parse (data, size, &tag, &tag_size, &tag_data)) {
    if (tag_size == 0 || tag == 0x0000)
      goto next;

    switch (tag) {
      case 0x3c0a:
        if (tag_size != 16)
          goto error;
        memcpy (&segment->instance_id, tag_data, 16);
        GST_DEBUG ("  instance id = %s",
            mxf_uuid_to_string (&segment->instance_id, str));
        break;
      case 0x3f0b:
        if (!mxf_fraction_parse (&segment->index_edit_rate, tag_data, tag_size))
          goto error;
        GST_DEBUG ("  index edit rate = %d/%d", segment->index_edit_rate.n,
            segment->index_edit_rate.d);
        break;
      case 0x3f0c:
        if (tag_size != 8)
          goto error;
        segment->index_start_position = GST_READ_UINT64_BE (tag_data);
        GST_DEBUG ("  index start position = %" G_GINT64_FORMAT,
            segment->index_start_position);
        break;
      case 0x3f0d:
        if (tag_size != 8)
          goto error;
        segment->index_duration = GST_READ_UINT64_BE (tag_data);
        GST_DEBUG ("  index duration = %" G_GINT64_FORMAT,
            segment->index_duration);
        break;
      case 0x3f05:
        if (tag_size != 4)
          goto error;
        segment->edit_unit_byte_count = GST_READ_UINT32_BE (tag_data);
        GST_DEBUG ("  edit unit byte count = %u",
            segment->edit_unit_byte_count);
        break;
      case 0x3f06:
        if (tag_size != 4)
          goto error;
        segment->index_sid = GST_READ_UINT32_BE (tag_data);
        GST_DEBUG ("  index sid = %u", segment->index_sid);
        break;
      case 0x3f07:
        if (tag_size != 4)
          goto error;
        segment->body_sid = GST_READ_UINT32_BE (tag_data);
        GST_DEBUG ("  body sid = %u", segment->body_sid);
        break;
      case 0x3f08:
        if (tag_size != 1)
          goto error;
        segment->slice_count = GST_READ_UINT8 (tag_data);
        GST_DEBUG ("  slice count = %u", segment->slice_count);
        break;
      case 0x3f0e:
        if (tag_size != 1)
          goto error;
        segment->pos_table_count = GST_READ_UINT8 (tag_data);
        GST_DEBUG ("  pos table count = %u", segment->pos_table_count);
        break;
      case 0x3f09:{
        guint len, i;

        if (tag_size < 8)
          goto error;

        len = GST_READ_UINT32_BE (tag_data);
        segment->n_delta_entries = len;
        GST_DEBUG ("  number of delta entries = %u", segment->n_delta_entries);
        if (len == 0)
          goto next;
        tag_data += 4;
        tag_size -= 4;

        if (GST_READ_UINT32_BE (tag_data) != 6)
          goto error;

        tag_data += 4;
        tag_size -= 4;

        if (tag_size < len * 6)
          goto error;

        segment->delta_entries = g_new (MXFDeltaEntry, len);

        for (i = 0; i < len; i++) {
          GST_DEBUG ("    delta entry %u:", i);

          segment->delta_entries[i].pos_table_index = GST_READ_UINT8 (tag_data);
          tag_data += 1;
          tag_size -= 1;
          GST_DEBUG ("    pos table index = %d",
              segment->delta_entries[i].pos_table_index);

          segment->delta_entries[i].slice = GST_READ_UINT8 (tag_data);
          tag_data += 1;
          tag_size -= 1;
          GST_DEBUG ("    slice = %u", segment->delta_entries[i].slice);

          segment->delta_entries[i].element_delta =
              GST_READ_UINT32_BE (tag_data);
          tag_data += 4;
          tag_size -= 4;
          GST_DEBUG ("    element delta = %u",
              segment->delta_entries[i].element_delta);
        }
        break;
      }
      case 0x3f0a:{
        guint len, i, j;

        if (tag_size < 8)
          goto error;

        len = GST_READ_UINT32_BE (tag_data);
        segment->n_index_entries = len;
        GST_DEBUG ("  number of index entries = %u", segment->n_index_entries);
        if (len == 0)
          goto next;
        tag_data += 4;
        tag_size -= 4;

        if (GST_READ_UINT32_BE (tag_data) !=
            (11 + 4 * segment->slice_count + 8 * segment->pos_table_count))
          goto error;

        tag_data += 4;
        tag_size -= 4;

        if (tag_size < len * 11)
          goto error;

        segment->index_entries = g_new0 (MXFIndexEntry, len);

        for (i = 0; i < len; i++) {
          MXFIndexEntry *entry = &segment->index_entries[i];

          GST_DEBUG ("    index entry %u:", i);

          entry->temporal_offset = GST_READ_UINT8 (tag_data);
          tag_data += 1;
          tag_size -= 1;
          GST_DEBUG ("    temporal offset = %d", entry->temporal_offset);

          entry->key_frame_offset = GST_READ_UINT8 (tag_data);
          tag_data += 1;
          tag_size -= 1;
          GST_DEBUG ("    keyframe offset = %d", entry->key_frame_offset);

          entry->flags = GST_READ_UINT8 (tag_data);
          tag_data += 1;
          tag_size -= 1;
          GST_DEBUG ("    flags = 0x%02x", entry->flags);

          entry->stream_offset = GST_READ_UINT64_BE (tag_data);
          tag_data += 8;
          tag_size -= 8;
          GST_DEBUG ("    stream offset = %" G_GUINT64_FORMAT,
              entry->stream_offset);

          entry->slice_offset = g_new0 (guint32, segment->slice_count);
          for (j = 0; j < segment->slice_count; j++) {
            entry->slice_offset[j] = GST_READ_UINT32_BE (tag_data);
            tag_data += 4;
            tag_size -= 4;
            GST_DEBUG ("    slice %u offset = %u", j, entry->slice_offset[j]);
          }

          entry->pos_table = g_new0 (MXFFraction, segment->pos_table_count);
          for (j = 0; j < segment->pos_table_count; j++) {
            mxf_fraction_parse (&entry->pos_table[j], tag_data, tag_size);
            tag_data += 8;
            tag_size -= 8;
            GST_DEBUG ("    pos table %u = %d/%d", j, entry->pos_table[j].n,
                entry->pos_table[j].d);
          }
        }
        break;
      }
      default:
        if (!primer->mappings) {
          GST_WARNING ("No valid primer pack for this partition");
        } else if (!mxf_local_tag_add_to_hash_table (primer, tag, tag_data,
                tag_size, &segment->other_tags)) {
          goto error;
        }
        break;
    }

  next:
    data += 4 + tag_size;
    size -= 4 + tag_size;
  }
  return TRUE;

error:
  GST_ERROR ("Invalid index table segment");
  return FALSE;
}

void
mxf_index_table_segment_reset (MXFIndexTableSegment * segment)
{
  guint i;

  g_return_if_fail (segment != NULL);

  for (i = 0; i < segment->n_index_entries; i++) {
    g_free (segment->index_entries[i].slice_offset);
    g_free (segment->index_entries[i].pos_table);
  }

  g_free (segment->index_entries);
  g_free (segment->delta_entries);

  if (segment->other_tags)
    g_hash_table_destroy (segment->other_tags);

  memset (segment, 0, sizeof (MXFIndexTableSegment));
}

/* SMPTE 377M 8.2 Table 1 and 2 */

static void
_mxf_mapping_ul_free (MXFUL * ul)
{
  g_slice_free (MXFUL, ul);
}

gboolean
mxf_primer_pack_parse (const MXFUL * ul, MXFPrimerPack * pack,
    const guint8 * data, guint size)
{
  guint i;
  guint32 n;

  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (size >= 8, FALSE);

  memset (pack, 0, sizeof (MXFPrimerPack));

  GST_DEBUG ("Parsing primer pack:");

  pack->mappings =
      g_hash_table_new_full (g_direct_hash, g_direct_equal,
      (GDestroyNotify) NULL, (GDestroyNotify) _mxf_mapping_ul_free);

  n = GST_READ_UINT32_BE (data);
  data += 4;

  GST_DEBUG ("  number of mappings = %u", n);

  if (GST_READ_UINT32_BE (data) != 18)
    goto error;
  data += 4;

  if (size < 8 + n * 18)
    goto error;

  for (i = 0; i < n; i++) {
    guint local_tag;
#ifndef GST_DISABLE_GST_DEBUG
    gchar str[48];
#endif
    MXFUL *uid;

    local_tag = GST_READ_UINT16_BE (data);
    data += 2;

    if (g_hash_table_lookup (pack->mappings, GUINT_TO_POINTER (local_tag)))
      continue;

    uid = g_slice_new (MXFUL);
    memcpy (uid, data, 16);
    data += 16;

    g_hash_table_insert (pack->mappings, GUINT_TO_POINTER (local_tag), uid);
    GST_DEBUG ("  Adding mapping = 0x%04x -> %s", local_tag,
        mxf_ul_to_string (uid, str));
  }

  return TRUE;

error:
  GST_DEBUG ("Invalid primer pack");
  mxf_primer_pack_reset (pack);
  return FALSE;
}

void
mxf_primer_pack_reset (MXFPrimerPack * pack)
{
  g_return_if_fail (pack != NULL);

  if (pack->mappings)
    g_hash_table_destroy (pack->mappings);
  if (pack->reverse_mappings)
    g_hash_table_destroy (pack->reverse_mappings);

  memset (pack, 0, sizeof (MXFPrimerPack));

  pack->next_free_tag = 0x8000;
}

guint16
mxf_primer_pack_add_mapping (MXFPrimerPack * primer, guint16 local_tag,
    const MXFUL * ul)
{
  MXFUL *uid;
#ifndef GST_DISABLE_GST_DEBUG
  gchar str[48];
#endif
  guint ltag_tmp = local_tag;

  if (primer->mappings == NULL) {
    primer->mappings = g_hash_table_new_full (g_direct_hash, g_direct_equal,
        (GDestroyNotify) NULL, (GDestroyNotify) _mxf_mapping_ul_free);
  }

  if (primer->reverse_mappings == NULL) {
    primer->reverse_mappings = g_hash_table_new_full ((GHashFunc) mxf_ul_hash,
        (GEqualFunc) mxf_ul_is_equal, (GDestroyNotify) _mxf_mapping_ul_free,
        (GDestroyNotify) NULL);
  }

  if (primer->next_free_tag == 0xffff && ltag_tmp == 0) {
    GST_ERROR ("Used too many dynamic tags");
    return 0;
  }

  if (ltag_tmp == 0) {
    guint tmp;

    tmp = GPOINTER_TO_UINT (g_hash_table_lookup (primer->reverse_mappings, ul));
    if (tmp == 0) {
      ltag_tmp = primer->next_free_tag;
      primer->next_free_tag++;
    }
  } else {
    if (g_hash_table_lookup (primer->mappings, GUINT_TO_POINTER (ltag_tmp)))
      return ltag_tmp;
  }

  g_assert (ltag_tmp != 0);

  uid = g_slice_new (MXFUL);
  memcpy (uid, ul, 16);

  GST_DEBUG ("Adding mapping = 0x%04x -> %s", ltag_tmp,
      mxf_ul_to_string (uid, str));
  g_hash_table_insert (primer->mappings, GUINT_TO_POINTER (ltag_tmp), uid);
  uid = g_slice_dup (MXFUL, uid);
  g_hash_table_insert (primer->reverse_mappings, uid,
      GUINT_TO_POINTER (ltag_tmp));

  return ltag_tmp;
}

GstBuffer *
mxf_primer_pack_to_buffer (const MXFPrimerPack * pack)
{
  guint slen;
  guint8 ber[9];
  GstBuffer *ret;
  GstMapInfo map;
  guint n;
  guint8 *data;

  if (pack->mappings)
    n = g_hash_table_size (pack->mappings);
  else
    n = 0;

  slen = mxf_ber_encode_size (8 + 18 * n, ber);

  ret = gst_buffer_new_and_alloc (16 + slen + 8 + 18 * n);
  gst_buffer_map (ret, &map, GST_MAP_WRITE);

  memcpy (map.data, MXF_UL (PRIMER_PACK), 16);
  memcpy (map.data + 16, &ber, slen);

  data = map.data + 16 + slen;

  GST_WRITE_UINT32_BE (data, n);
  GST_WRITE_UINT32_BE (data + 4, 18);
  data += 8;

  if (pack->mappings) {
    guint local_tag;
    MXFUL *ul;
    GHashTableIter iter;

    g_hash_table_iter_init (&iter, pack->mappings);

    while (g_hash_table_iter_next (&iter, (gpointer) & local_tag,
            (gpointer) & ul)) {
      GST_WRITE_UINT16_BE (data, local_tag);
      memcpy (data + 2, ul, 16);
      data += 18;
    }
  }

  gst_buffer_unmap (ret, &map);

  return ret;
}

/* structural metadata parsing */

gboolean
mxf_local_tag_parse (const guint8 * data, guint size, guint16 * tag,
    guint16 * tag_size, const guint8 ** tag_data)
{
  g_return_val_if_fail (data != NULL, FALSE);

  if (size < 4)
    return FALSE;

  *tag = GST_READ_UINT16_BE (data);
  *tag_size = GST_READ_UINT16_BE (data + 2);

  if (size < 4 + *tag_size)
    return FALSE;

  *tag_data = data + 4;

  return TRUE;
}

void
mxf_local_tag_free (MXFLocalTag * tag)
{
  if (tag->g_slice)
    g_slice_free1 (tag->size, tag->data);
  else
    g_free (tag->data);
  g_slice_free (MXFLocalTag, tag);
}

gboolean
mxf_local_tag_add_to_hash_table (const MXFPrimerPack * primer,
    guint16 tag, const guint8 * tag_data, guint16 tag_size,
    GHashTable ** hash_table)
{
  MXFLocalTag *local_tag;
  MXFUL *ul;

  g_return_val_if_fail (primer != NULL, FALSE);
  g_return_val_if_fail (tag_data != NULL, FALSE);
  g_return_val_if_fail (hash_table != NULL, FALSE);
  g_return_val_if_fail (primer->mappings != NULL, FALSE);

  if (*hash_table == NULL)
    *hash_table =
        g_hash_table_new_full ((GHashFunc) mxf_ul_hash,
        (GEqualFunc) mxf_ul_is_equal, (GDestroyNotify) NULL,
        (GDestroyNotify) mxf_local_tag_free);

  g_return_val_if_fail (*hash_table != NULL, FALSE);

  ul = (MXFUL *) g_hash_table_lookup (primer->mappings,
      GUINT_TO_POINTER (((guint) tag)));

  if (ul) {
#ifndef GST_DISABLE_GST_DEBUG
    gchar str[48];
#endif

    GST_DEBUG ("Adding local tag 0x%04x with UL %s and size %u", tag,
        mxf_ul_to_string (ul, str), tag_size);

    local_tag = g_slice_new0 (MXFLocalTag);
    memcpy (&local_tag->ul, ul, sizeof (MXFUL));
    local_tag->size = tag_size;
    local_tag->data = g_memdup (tag_data, tag_size);
    local_tag->g_slice = FALSE;

    g_hash_table_insert (*hash_table, &local_tag->ul, local_tag);
  } else {
    GST_WARNING ("Local tag with no entry in primer pack: 0x%04x", tag);
  }

  return TRUE;
}

gboolean
mxf_local_tag_insert (MXFLocalTag * tag, GHashTable ** hash_table)
{
#ifndef GST_DISABLE_GST_DEBUG
  gchar str[48];
#endif

  g_return_val_if_fail (tag != NULL, FALSE);
  g_return_val_if_fail (hash_table != NULL, FALSE);

  if (*hash_table == NULL)
    *hash_table =
        g_hash_table_new_full ((GHashFunc) mxf_ul_hash,
        (GEqualFunc) mxf_ul_is_equal, (GDestroyNotify) NULL,
        (GDestroyNotify) mxf_local_tag_free);

  g_return_val_if_fail (*hash_table != NULL, FALSE);

  GST_DEBUG ("Adding local tag with UL %s and size %u",
      mxf_ul_to_string (&tag->ul, str), tag->size);

  g_hash_table_insert (*hash_table, &tag->ul, tag);

  return TRUE;
}
