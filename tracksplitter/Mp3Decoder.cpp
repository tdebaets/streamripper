// Mp3Decoder.cpp: implementation of the CMp3Decoder class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Mp3Decoder.h"
#include <assert.h>
#include "log.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMp3Decoder::CMp3Decoder()
{
	m_pmp3Buf = new char[MP3BUFSIZE];
	m_mp3BufPos = 0;
	m_bytesDecoded = 0;
	mad_stream_init(&m_stream);
	mad_frame_init(&m_frame);
	mad_synth_init(&m_synth);
}

CMp3Decoder::~CMp3Decoder()
{
	delete [] m_pmp3Buf;
}


void CMp3Decoder::FeedStream(char *pdata, long size)
{
	assert(pdata);
	LogMessage(LOG_DEBUG, "feed_stream: size=%d, mp3pos=%d", size, m_mp3BufPos);
	if (m_mp3BufPos+size > MP3BUFSIZE)
	{
		assert(0);
		throw CMp3Decoder_BufferFull();
	}

	memcpy(&m_pmp3Buf[m_mp3BufPos], pdata, size);
	m_mp3BufPos += size;
}

long CMp3Decoder::GetStreamSize()
{
	return m_mp3BufPos;
}

static signed int scale(mad_fixed_t sample)
{
	// round 
	sample += (1L << (MAD_F_FRACBITS - 16));

	// clip 
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	// quantize 
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

void CMp3Decoder::HandleMp3Output()
{
	unsigned int	nchannels, nsamples;
	signed short	samples[8196];
	struct mad_pcm *pcm;

	mad_fixed_t const *left_ch, *right_ch;
	static short lastSample = 0;
	static signed int sample;

	pcm = &m_synth.pcm;

	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	long	in_samplerate = m_sampleRate;
	long	in_nch = m_nch;

	int	samplecount = 0;
	for (int i=0;i<nsamples;i++) {
		samples[samplecount] = scale(*left_ch++);

		samplecount++;
		if (nchannels == 2) 
		{
			samples[samplecount] = scale(*right_ch++);
			samplecount++;
		}
	}
	LogMessage(LOG_DEBUG, "handle_output - nsamples = %d, nchannels = %d, in_samplerate = %d, in_nch = %d", nsamples, nchannels, in_samplerate, in_nch);
	if (m_pdecodeHandler)
	{
		m_pdecodeHandler->OnMp3Data((signed short*)&samples, nsamples, nchannels, in_samplerate, GetBitRate());
	}
}


int CMp3Decoder::Decode()
{
	struct mad_stream* pstream = &m_stream;
	struct mad_frame* pframe = &m_frame;
	struct mad_synth* psynth = &m_synth;
	int ret = 1;
	unsigned long startbytes = m_bytesDecoded;

	mad_stream_buffer(pstream, (const unsigned char*)m_pmp3Buf, m_mp3BufPos);
	
	while(1)
	{
		long ret = mad_frame_decode(pframe, pstream);
		long diff = (long)pstream->this_frame-(long)m_pmp3Buf;
		m_bytesDecoded = startbytes+diff;

		if (pstream->error != MAD_ERROR_NONE) {
			if (pstream->error == MAD_ERROR_BUFLEN ||
				!MAD_RECOVERABLE(pstream->error))
				break;
			//LogMessage(LOG_ERROR, "pstream Error: 0x%x", pstream->error);
			//pstream->error = MAD_ERROR_NONE;
		}
		
		if (pstream->error == MAD_ERROR_NONE) {
			mad_synth_frame(psynth, pframe);
			m_sampleRate = pframe->header.samplerate;
			m_bitRate = pframe->header.bitrate;
			switch (pframe->header.mode) {
				case MAD_MODE_SINGLE_CHANNEL:
					m_nch = 1;
					break;
				case MAD_MODE_DUAL_CHANNEL:
				case MAD_MODE_JOINT_STEREO:
				case MAD_MODE_STEREO:
					m_nch = 2;
					break;
			}
			HandleMp3Output();
		}
	}

	pstream->error = MAD_ERROR_NONE;
	
	if (!pstream->next_frame) 
		assert(0);

	long bufferleft = (long)&(m_pmp3Buf[m_mp3BufPos]) - 
					  (long)pstream->next_frame;
	assert(bufferleft > 0);

	// move the mp3buf to the begining
	memmove(m_pmp3Buf, 
			&m_pmp3Buf[m_mp3BufPos-bufferleft], 
			bufferleft);
	m_mp3BufPos = bufferleft;
	LogMessage(LOG_DEBUG, "---------");

	return ret;
}

