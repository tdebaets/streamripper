/* findsep.c - jonclegg@yahoo.com
 * library routines for find silent points in mp3 data
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if WIN32
#include "decoder.h"
#include <windows.h>
#include "memory_input.h"
#include "audio_output.h"
#else
#include "mpg123.h"
#include "mpglib.h"
#endif

#include "mpeg.h"
#include "findsep.h"
#include "util.h"
#include "types.h"
#include "cbuffer.h"


#define MP3_BUFFER_SIZE			(1024*10)
#define REALLOC_CHUNKS			(0xFF)
#define MIN_GOOD_FRAMES			3
#define RMS_WINDOWS				1000
#define MIN_SILENT_RMS			500
#define MIN_SILENT_RMS_FRAMES	50


/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	findsep_silence(const u_char *buffer, const u_long size, u_long *pos);

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code	decode_buffer(const u_char *buffer, long size, short **pcm_buffer, u_long *pcmsize);
static u_long		find_silence(const short *buffer, const u_long size, const int numwindows);
static void			convert_to_mono(short *pcmdata, u_long size, u_long *newsize);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
typedef struct RMSst
{
	u_long pos;
	double val;
} RMS;

/*
 * findsep_silence() you give it some mp3 data, it'll tell you where the best
 * silent point in that data is. it's not perfect however works most of the time.
 * first we decode the mp3 data to raw PCM, then convert the PCM to mono (for 
 * fast processing). then split up the PCM data into RMS_WINDOWS windows or peices
 * adverage each window and pick the one with the lowest value. visualy it might look 
 * like this:
 * 
 * | --20- | --10- | --5-- | --30- | --50- | --20- | --10- | 
 * 
 * Where | is a window boundy, '-' are mp3 data and the numbers are the averages 
 * for each frame. in this case we would pick the third one. 
 *
 * after we find the silent point in the PCM data we figure out aprox where that 
 * would have been in the MP3 data with a simple ratio. then seek to the first MP3
 * header after that point and return that value
 */
error_code findsep_silence(const u_char *buffer, const u_long size, u_long *pos)
{
	short *pcm_buffer = NULL;
	u_long pcmsize, monosize;
	u_long mypos = 0, pcmpos = 0;
	int offset = 0;
	float ratio = 0;
	int ret;

	/*
	 * MIN_GOOD_FRAMES is a number that seems to be about right to
	 * qualify a peice of mp3 data as good. if we can't find a header
	 * thats consistant MIN_GOOD_FRAMES times then we have some pretty 
	 * fucked up data. The reason we do this is that some mp3 
	 * decoders cough*mpglib*cough are pron to failure  and any 
	 * help they can get in figuring out what to do is a good thing
	 */
	if (mpeg_find_first_header(buffer, size, MIN_GOOD_FRAMES*2, &offset) != SR_SUCCESS)
	{
		*pos = 0;
		return SR_ERROR_CANT_DECODE_MP3; // Not really, but what else can we do?
	}

	/*
	 * Try to decode the mp3 data, currently on Wintel boxens we use xaudio
	 * a free/comercial mp3 decoding library which is of very high quality (it doesn't
	 * crash and works all the time). under *nix's i expect people to compile it
	 * themselfs so i'm using mpglib, which really sux. however i think i've done
	 * enough massaging now to make it somewhat stable.
	 */
	debug_printf("\nstart decode\n");
	if ((ret = decode_buffer(buffer+offset, size-offset, &pcm_buffer, &pcmsize)) != SR_SUCCESS)
	{
		*pos = offset; // better then nothing.
		return ret; 
	}
	debug_printf("end decode\n");

	// Convert to mono
	convert_to_mono(pcm_buffer, pcmsize, &monosize);
	pcmsize = monosize;

	ratio = (float)size / pcmsize;

	// Find the lowest RMS 
	pcmpos = find_silence(pcm_buffer, pcmsize, RMS_WINDOWS);

	// mypos is an aprox silent position based of the PCM data
	mypos = (u_long)(((float)pcmpos)*ratio);
	
	// Find the first valid MPEG header
	offset = 0;
	mpeg_find_first_header(buffer+mypos, size-mypos, MIN_GOOD_FRAMES, &offset);
	*pos = mypos + offset;

	// Just to make sure nothing stupid happened
	if (*pos > size)
		*pos = size;

	free(pcm_buffer);

	return SR_SUCCESS;
	
}

void convert_to_mono(short *pcmdata, u_long size, u_long *newsize)
{
	u_long i, y, nsize;
	short *mono = malloc((size/2) * sizeof(short));

	for(i = 1, y = 0; i < size; i += 2, y++)
	{
		mono[y] = (pcmdata[i-1] + pcmdata[i]) / 2;

	}
	nsize = size/2;
	memset(pcmdata, 0, size * sizeof(short));
	memcpy(pcmdata, mono, nsize * sizeof(short));
	*newsize = nsize;

	free(mono);
}

/*
 * This is explained more above, the only thing to add is that 
 * RMS means "root mean sqaure" and is used by EE to figure out 
 * power levels in signals. but to put it simply it's a fancy average.
 */
u_long find_silence(const short *buffer, const u_long size, const int numwindows)
{

	const int window_size = size / numwindows;
	int rms_vals_count; 
	RMS *rms_vals;
	u_long x, pos;
	int i, n;
	int min;	
	
	if (window_size == 0)
		return 0;

	rms_vals_count = size / window_size;
	rms_vals = calloc(rms_vals_count, sizeof(RMS));

	x = 0;

	// Get RMS values
	for(i = 0; i < rms_vals_count; i++)
	{
		n = window_size;
		while(n--)
		{
			rms_vals[i].val += pow(buffer[x++], 2);
		}
		rms_vals[i].val = sqrt(rms_vals[i].val / window_size);
		rms_vals[i].pos = x - (window_size/2);
	}

	for(i = 0, min = 0; i < rms_vals_count; i++)
	{
		if (rms_vals[i].val < rms_vals[min].val)
			min = i;
	}

	// Get the lowest value within the winning window
	pos = rms_vals[min].pos;
	min = pos;	// Min is now an index of buffer, not rms_vals
	for(i = pos; i < (signed)pos+window_size; i++)
	{
		if (buffer[i] < buffer[min])
			min = i;
	}
	pos = min;


	free(rms_vals);
//	debug_printf("\npcmpos: %ld\n", pos);
	return pos;
}

/*
 * I'm sick of trying to make mpglib work correctly. so i'm going back to using xaudio
 * for the win32 build. everything else will still use mpglib.
 */
#ifdef WIN32
// Note: size must be larger then MP3_BUFFER_SIZE
error_code decode_buffer(const u_char *buffer, long size, short **pcm_buffer, u_long *pcmsize)
{
    int		status; 
    u_long	bufferpos = 0;
    u_char	*pcm, *my_pcm_buf;
    u_long	pcm_size = 0;
    int		mp3buffersize = MP3_BUFFER_SIZE;
    int		pcm_chunk_size = 0;
    u_long	pcm_real_size;
	int		ret;
	XA_InputModule	module;
	XA_DecoderInfo	*decoder = NULL;
   
	// Init xaudio
	if (mp3buffersize > size)
		return SR_ERROR_CANT_DECODE_MP3;

	if (decoder_new(&decoder) != XA_SUCCESS)
		return SR_ERROR_CANT_INIT_XAUDIO;

	memory_input_module_register(&module);
	decoder_input_module_register(decoder, &module);

	ret = decoder_input_new(decoder, NULL, XA_DECODER_INPUT_AUTOSELECT);
    if (ret != XA_SUCCESS)
    {
            decoder_delete(decoder);
            return SR_ERROR_CANT_INIT_XAUDIO;
    }

    if (decoder_input_open(decoder) != XA_SUCCESS)
    {
            decoder_delete(decoder);
            return SR_ERROR_CANT_INIT_XAUDIO;
    }
    memory_input_flush(decoder->input->device);


	// Decode mp3
    do 
    {

		if (bufferpos == (unsigned)size)
			break;

		if (bufferpos > (unsigned)(size-MP3_BUFFER_SIZE))
			mp3buffersize = size - bufferpos;

		memory_input_feed(decoder->input->device, &buffer[bufferpos], mp3buffersize);

		do {
		status = decoder_decode(decoder, NULL);
		if (status == XA_SUCCESS) 
		{
			if (!pcm_chunk_size)
			{
				pcm_chunk_size = decoder->output_buffer->size;
				pcm_real_size = REALLOC_CHUNKS*pcm_chunk_size;
				my_pcm_buf = malloc(pcm_real_size);
			}

			pcm = (u_char *)decoder->output_buffer->pcm_samples;
			if (*pcm)                //forget until we start getting none null frames
			{
				pcm_size += pcm_chunk_size;
				if (pcm_size % pcm_real_size == 0)
				{
					pcm_real_size += (REALLOC_CHUNKS*pcm_chunk_size);
					my_pcm_buf = realloc(my_pcm_buf, pcm_real_size);
				}
				memcpy((my_pcm_buf)+(pcm_size-pcm_chunk_size), pcm, pcm_chunk_size);
			}
                    }
        } while(status == XA_SUCCESS ||
                        status == XA_ERROR_INVALID_FRAME);
        bufferpos += mp3buffersize;

    } while(status == XA_SUCCESS ||
                status == XA_ERROR_TIMEOUT);


    if (decoder_input_close(decoder) != XA_SUCCESS)
    {
        decoder_delete(decoder);
        return SR_ERROR_CANT_INIT_XAUDIO;
    }

    decoder_delete(decoder);

    *pcm_buffer = (short *)my_pcm_buf;
    *pcmsize = pcm_size / 2;
    return SR_SUCCESS;
}
#else

error_code decode_buffer(const u_char *buffer, long size, short **pcm_buffer, u_long *pcmsize)
{
        int decode_status;
        u_long pcm_max_size = size*50;  // Better not ever get better then 100x compression
        int decoded_size;
        BOOL pcm_started = FALSE;
        char *pcmbuf = (char *)malloc(pcm_max_size);
        struct mpstr mp;
        int frame_pos = 0;
        int mp3_read = 0;

        *pcmsize = 0;
        InitMP3(&mp);
        memset(pcmbuf, 0, pcm_max_size);
        // Load up the mp3 data into the decoder
        decode_status = decodeMP3(&mp, (char*)buffer, size, pcmbuf, pcm_max_size, &decoded_size);
        do
        {
                // and roll it out into out buffer
				decoded_size = 0;
                decode_status = decodeMP3(&mp, NULL, 0, pcmbuf+*pcmsize, pcm_max_size-*pcmsize, &decoded_size);
                if (decode_status == MP3_ERR)
                {
                        // reset everything, advance to the next buffer.
                        mp3_read = (size-mp.bsize); // bsize is how much of the buffer is left
                        printf("\nmpglib error at: %d\n", mp.bsize);
                        buffer += mp3_read;
                        size -= mp3_read;
                        mpeg_find_first_header(buffer, size, 5, &frame_pos);
                        buffer += frame_pos;
                        size -= frame_pos;
						decoded_size = 0;
                        ExitMP3(&mp);
                        InitMP3(&mp);
                        decode_status = decodeMP3(&mp, (char*)buffer, size, pcmbuf+*pcmsize,
                                                pcm_max_size - *pcmsize, &decoded_size);

                        // Probably don't need this, but it might come up
                        if (mp.framesize == 0 && frame_pos == 0 &&
                            decoded_size == 0 && decode_status == MP3_ERR) // we must be at the end
                        {
                                break;
                        }
                        continue;
                }

                // Don't record until we start getting some PCM data
                //printf("decode_status = %d, decoded_size = %d\n", decode_status, decoded_size);
                if (pcm_started)
                        *pcmsize += decoded_size;
                else if (*pcmbuf)
                        pcm_started = TRUE;

                if (*pcmsize > pcm_max_size)
                {
                        free(pcmbuf);
                        return SR_ERROR_PCM_BUFFER_TO_SMALL;
                }
        } while(mp.bsize && decode_status != MP3_NEED_MORE);

        ExitMP3(&mp);
        *pcm_buffer = (short*)realloc((void *)pcmbuf, (size_t)*pcmsize);
        if (*pcm_buffer == NULL && *pcmsize != 0)
                return SR_ERROR_CANT_ALLOC_MEMORY;
        *pcmsize /= 2;  // Convert back to short

        return SR_SUCCESS;
}



#endif
