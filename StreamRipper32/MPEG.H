#ifndef MPEG_H
#define MPEG_H

#include "types.h"

typedef struct mp3_headerSt
{
  int lay;
  int version;
  int error_protection;
  int bitrate_index;
  int sampling_frequency;
  int padding;
  int extension;
  int mode;
  int mode_ext;
  int copyright;
  int original;
  int emphasis;
  int stereo;
} mp3_header_t;


BOOL find_first_header(unsigned char* pbuff, int size, int min_good_frames, int *frame_pos, mp3_header_t *mp3_header);
extern int get_frame_size(mp3_header_t *mh);

#endif

