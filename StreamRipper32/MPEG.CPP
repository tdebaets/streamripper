//mpeg.c

#include <stdio.h>
#include <string.h>
#include "mpeg.h"
#include "error.h"

#define MPG_MD_MONO 3

BOOL find_first_header(unsigned char* pbuff, int size, int min_good_frames, int *frame_pos, mp3_header_t *mp3_header);
int get_frame_size(mp3_header_t *mh);
BOOL get_header(unsigned char *buff, mp3_header_t *mh);
BOOL header_sane(mp3_header_t *mh);
BOOL sameHeader(mp3_header_t *mh1, mp3_header_t *mh2);
unsigned int decode_bitrate(mp3_header_t *mh);
unsigned int decode_freq(mp3_header_t *mh);

unsigned int bitrates[3][3][15] =
{
  {
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
  },
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  },
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  }
};

unsigned int s_freq[3][4] =
{
    {44100, 48000, 32000, 0},
    {22050, 24000, 16000, 0},
    {11025, 8000, 8000, 0}
};

char *mode_names[5] = {"stereo", "j-stereo", "dual-ch", "single-ch", "multi-ch"};
char *layer_names[3] = {"I", "II", "III"};
char *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};
char *version_nums[3] = {"1", "2", "2.5"};

typedef struct frameSt
{
	int count;
	int frame_size;
	int first_pos;
	int first_appearance;
} frame_t;


//finds the first header good for min_good_frames and returns the position and mp3_header.
BOOL find_first_header(unsigned char* pbuff, int size, int min_good_frames, int *header_pos, mp3_header_t *mp3_header)
{
	int i, n;
	unsigned char *buff = pbuff;
	unsigned char *buff2;
	mp3_header_t mh1, mh2;
	int frame_size;
	int remaining;

	for(i = 0; i < size; i++, buff++)
	{
		if (*buff == 255)
		{
			get_header(buff, &mh1);	
			if (!header_sane(&mh1))
				continue;
			scream(VERBOSE, "header is sane at: %d\n", i);

			frame_size = get_frame_size(&mh1);
			if (frame_size < 21)
				continue;

			scream(VERBOSE, "frame size ok with: %d\n", frame_size);

			buff2 = buff + frame_size;
			remaining = size - i - frame_size;
			for(n = 1; (n < min_good_frames) && (remaining >= 4); n++)
			{
				get_header(buff2, &mh2);
				if (!header_sane(&mh2)) break;
				if (!sameHeader(&mh1, &mh2)) break;

				remaining -= frame_size;
				buff2 += frame_size;
			}
			if (n == min_good_frames)
			{
				memcpy(mp3_header, &mh1, sizeof(mh1));
				*header_pos = i;		
				return TRUE;
			}
		}	
	}

	return FALSE;
}
			

//checks if the headers are the same
BOOL sameHeader(mp3_header_t *mh1, mp3_header_t *mh2)
{
	if ((mh1->lay 			== mh2->lay) &&
	    (mh1->version 		== mh2->version) &&
	    (mh1->error_protection	== mh2->error_protection) &&
	    (mh1->bitrate_index		== mh2->bitrate_index) &&
	    (mh1->sampling_frequency	== mh2->sampling_frequency) &&
	    (mh1->padding		== mh2->padding) &&
	    (mh1->extension		== mh2->extension) &&
	    (mh1->mode			== mh2->mode) &&
	    (mh1->mode_ext		== mh2->mode_ext) &&
	    (mh1->copyright		== mh2->copyright) &&
	    (mh1->original		== mh2->original) &&
	    (mh1->emphasis		== mh2->emphasis) &&
	    (mh1->stereo		== mh2->stereo))
		return TRUE;
	else
		return FALSE;
}            	

//calcualtes the frame size based of a mp3_header
int get_frame_size(mp3_header_t *mh)
{
	long mult;

	if (decode_bitrate(mh) == 0)
		return 0;

	if (mh->version != 1)
		mult = mh->lay == 1 ? 48000 : 144000;
	else
		mult = mh->lay == 1 ? 24000 : 72000;

	return mult * decode_bitrate(mh) / decode_freq(mh) + mh->padding;
}

BOOL get_header(unsigned char *buff, mp3_header_t *mh)
{
	unsigned char *buffer;
 	size_t 	temp;


	memset(mh, 0, sizeof(mh)); 
	buffer = buff;
	temp = ((buffer[0] << 4) & 0xFF0) | ((buffer[1] >> 4) & 0xE);
  
	if (temp != 0xFFE) 
	{
     		return FALSE;
	} 
	else 
	{
		switch ((buffer[1] >> 3 & 0x3))
		{
			case 3:
				mh->version = 0;
	  			break;
			case 2:
	  			mh->version = 1;
	  			break;
			case 0:
	  			mh->version = 2;
	  			break;
			default:
	  			return -1;
	  			break;
      		}
      		mh->lay = 4 - ((buffer[1] >> 1) & 0x3);
      		mh->error_protection = !(buffer[1] & 0x1);
      		mh->bitrate_index = (buffer[2] >> 4) & 0x0F;
      		mh->sampling_frequency = (buffer[2] >> 2) & 0x3;
      		mh->padding = (buffer[2] >> 1) & 0x01;
      		mh->extension = buffer[2] & 0x01;
      		mh->mode = (buffer[3] >> 6) & 0x3;
      		mh->mode_ext = (buffer[3] >> 4) & 0x03;
      		mh->copyright = (buffer[3] >> 3) & 0x01;
      		mh->original = (buffer[3] >> 2) & 0x1;
      		mh->emphasis = (buffer[3]) & 0x3;
      		mh->stereo = (mh->mode == MPG_MD_MONO) ? 1 : 2;

		return TRUE;
    }
}

unsigned int decode_bitrate(mp3_header_t *mh)
{
	return bitrates[mh->version][mh->lay-1][mh->bitrate_index];
}

unsigned int decode_freq(mp3_header_t *mh)
{
	return s_freq[mh->version][mh->sampling_frequency];
} 

BOOL header_sane(mp3_header_t *mh)
{
	if ( ((mh->lay > 1) && (mh->lay < 4)) &&
	     ((mh->bitrate_index > 0) && (mh->bitrate_index < 15)) &&
	     ((mh->sampling_frequency >= 0) && (mh->sampling_frequency < 3)) &&
	     ((mh->version >= 0) && (mh->version < 3)))
		return TRUE;

	return FALSE;	
}
