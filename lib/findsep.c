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
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "findsep.h"
#include "util.h"
#include "types.h"
#include "cbuffer.h"
#include "debug.h"

#include "mad.h"	//mpeg library

#define MIN_RMS_SILENCE		100
#define MAX_RMS_SILENCE		32767 //max short
#define NUM_SILTRACKERS		30
#define READSIZE	2000

typedef struct SILENCETRACKERst
{
	long insilencecount;
	double silencevol;
	long silencestart;
	BOOL foundsil;
} SILENCETRACKER;

typedef struct DECODE_STRUCTst
{
	unsigned char* mpgbuf;
	long  mpgsize;
	long  mpgpos;
	long  psilence;
	long  pcmpos;
	long  samplerate; 
	SILENCETRACKER siltrackers[NUM_SILTRACKERS];
} DECODE_STRUCT;


/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code findsep_silence(const u_char* mpgbuf, long mpgsize, u_long* psilence);

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static void init_siltrackers(SILENCETRACKER* siltrackers);
static enum mad_flow input(void *data, struct mad_stream *ms);
static void search_for_silence(DECODE_STRUCT *ds, double vol);
static signed int scale(mad_fixed_t sample);
static enum mad_flow output(void *data, struct mad_header const *header,
				     struct mad_pcm *pcm);
static enum mad_flow output(void *data, struct mad_header const *header,
				     struct mad_pcm *pcm);
static enum mad_flow error(void *data, struct mad_stream *ms, struct mad_frame *frame);
static enum mad_flow header(void *data, struct mad_header const *pheader);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/

error_code findsep_silence(const u_char* mpgbuf, long mpgsize, u_long* psilence)
{
	DECODE_STRUCT ds;
	struct mad_decoder decoder;
	int result;
	long silstart;
	int i;
	
	ds.mpgbuf = (unsigned char*)mpgbuf;
	ds.mpgsize = mpgsize;
	ds.psilence = 0;
	ds.pcmpos = 0;
	ds.mpgpos= 0;
	ds.samplerate = 0;

	init_siltrackers(ds.siltrackers);

#ifdef _DEBUG_
	{
		FILE* fp = fopen("dump.mp3", "wb");
		fwrite(mpgbuf, mpgsize, 1, fp);
		fclose(fp);
	}
#endif

	/* initialize and start decoder */
	mad_decoder_init(&decoder, &ds,
		   input /* input */,

		   header/* header */,
		   NULL /* filter */,
		   output /* output */,
		   error /* error */,
		   NULL /* message */);

	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	mad_decoder_finish(&decoder);

	DEBUG2(("total length: %d\n", ds.pcmpos));

	assert(ds.mpgsize != 0);
	silstart = ds.mpgsize/2;
	for(i = 0; i < NUM_SILTRACKERS; i++)
	{
		DEBUG2(("i=%d, start=%d\n", i, ds.siltrackers[i].silencestart));
		if (ds.siltrackers[i].foundsil)
		{
			DEBUG2(( "found!\n" ));
			silstart = ds.siltrackers[i].silencestart;
			break;
		}

	}

	assert(i != NUM_SILTRACKERS);
	if (i == NUM_SILTRACKERS)
		printf("\nwarning: no silence found between tracks\n");
	*psilence = silstart;
	return SR_SUCCESS;

//	return SR_ERROR_CANT_DECODE_MP3; // Not really, but what else can we do?
}

void init_siltrackers(SILENCETRACKER* siltrackers)
{
	int i;
	long stepsize = (MAX_RMS_SILENCE - MIN_RMS_SILENCE) / (NUM_SILTRACKERS-1);
	long rms = MIN_RMS_SILENCE;
	for(i = 0; i < NUM_SILTRACKERS; i++, rms += stepsize)
	{
		siltrackers[i].foundsil = 0;
		siltrackers[i].silencestart = 0;
		siltrackers[i].insilencecount = 0;
		siltrackers[i].silencevol = rms;
	}
}

enum mad_flow input(void *data, struct mad_stream *ms)
{
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
	long frameoffset = 0;
	long espnextpos = ds->mpgpos+READSIZE;

	if (espnextpos > ds->mpgsize)
	{
		return MAD_FLOW_STOP;
	}

	if (ms->next_frame) 
	{
		frameoffset = &(ds->mpgbuf[ds->mpgpos]) - ms->next_frame;
		assert(frameoffset != READSIZE);
	}

	mad_stream_buffer(ms,
		(const unsigned char*)(ds->mpgbuf+ds->mpgpos)-frameoffset, 
		READSIZE);
	ds->mpgpos += READSIZE - frameoffset;

	return MAD_FLOW_CONTINUE;
}


void search_for_silence(DECODE_STRUCT *ds, double vol)
{
	int i;
	for(i = 0; i < NUM_SILTRACKERS; i++)
	{
		SILENCETRACKER *pstracker = &ds->siltrackers[i];

		if (pstracker->foundsil)
			continue;

		if (vol < pstracker->silencevol)
		{
			if (pstracker->insilencecount == 0)
				pstracker->silencestart = ds->mpgpos;
			pstracker->insilencecount++;
		}
		else
		{
			pstracker->insilencecount = 0;
		}

		if (pstracker->insilencecount > ds->samplerate)
		{
			pstracker->foundsil = TRUE;
		}
	}

}

signed int scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

enum mad_flow output(void *data, struct mad_header const *header,
				     struct mad_pcm *pcm)
{
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	static short lastSample = 0;
	static signed int sample;
	double v;
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;

	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	while(nsamples--)
	{
		/* output sample(s) in 16-bit signed little-endian PCM */
		lastSample = sample;
		sample = (short)scale(*left_ch++);
		//	fwrite(&sample, sizeof(short), 1, fp);

		if (nchannels == 2) 
		{
			// make mono
			sample = (sample+scale(*right_ch++))/2;
		}

		//
		// get the instantanous volume
		//
		v = (lastSample*lastSample)+(sample*sample);
		v = sqrt(v / 2);
		search_for_silence(ds, v);
	}
	

	return MAD_FLOW_CONTINUE;
}

static enum 
mad_flow header(void *data, struct mad_header const *pheader)
{
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
	if (!ds->samplerate)
		ds->samplerate = pheader->samplerate;
	return MAD_FLOW_CONTINUE;
}

enum mad_flow error(void *data, struct mad_stream *ms, struct mad_frame *frame)
{
	if (MAD_RECOVERABLE(ms->error)) 
	{
		DEBUG2(( "mad error 0x%04x\n", ms->error));
		return MAD_FLOW_CONTINUE;
	}

	DEBUG0(( "unrecoverable mad error 0x%04x\n", ms->error));
	return MAD_FLOW_BREAK;
}

