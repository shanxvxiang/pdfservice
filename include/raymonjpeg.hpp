
#ifndef __RAYMON_SHAN_JPEG_H
#define __RAYMON_SHAN_JPEG_H

#include <malloc.h>

#include "jpeglib.h"

// the caller should free(*outbuffer)
char *RGBX2Jpeg (int width, int height, const void *inbuffer,
		unsigned char **outbuffer, unsigned long *outlength, int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  JSAMPLE* jinbuffer = (JSAMPLE*)inbuffer;
  int row_stride;

  cinfo.err = jpeg_std_error(&jerr);;
  jpeg_create_compress(&cinfo);

  jpeg_mem_dest(&cinfo, outbuffer, outlength);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = 4;
  cinfo.in_color_space = JCS_EXT_BGRX;
  jpeg_set_defaults(&cinfo);

  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  row_stride = width * 4;
  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = & jinbuffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  return NULL;
}

#endif  // __RAYMON_SHAN_JPEG_H
