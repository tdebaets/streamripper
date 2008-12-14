// TrackSplitter1.cpp: implementation of the CTrackSplitter class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrackSplitter.h"


//-------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------
CTrackSplitter::CTrackSplitter()
:	m_fpIn(NULL),
	m_curFileIndex(1),
	m_silencestart(0),
	m_foundSilence(false),
	m_insilencecount(0),
	m_minTrackLen(MIN_TRACK_LEN),
	m_silencevol(DEFAULT_RMS_SILENCE),
	m_pEventHandler(NULL)
{

}

//-------------------------------------------------------------------------
// SplitTrack
//-------------------------------------------------------------------------
void CTrackSplitter::SplitTrack (const char* track)
{

	long			bytesread = 0;
	if (!track || !track[0])
	{
		throw CTrackSplitter_CantOpenFile();
	}

	m_decoder.SetDecodeHandler(*this);
	m_fpIn = fopen(track, "rb");
	if (!m_fpIn)
	{
		throw CTrackSplitter_CantOpenFile();
	}

	//-------------------------------------------------------------------------
	// main loop
	//-------------------------------------------------------------------------
	while(!feof(m_fpIn))
	{
		char buf[MP3_READ_SIZE];
		long readsize = MP3_READ_SIZE;
		long ret;
		while(m_decoder.GetStreamSize() <= MIN_MP3_STREAM_SIZE)
		{
			ret = fread(buf, 1, MP3_READ_SIZE, m_fpIn);
			if (ret < readsize)
				readsize = ret;
			if (ret == 0)
				break;
			bytesread += ret;
			m_decoder.FeedStream(buf, ret);
		}
		if (m_pEventHandler)
			m_pEventHandler->OnDataRead(bytesread);
		
		m_decoder.Decode();
		
		if (m_foundSilence)
		{
			m_foundSilence = false;
			m_insilencecount = 0;
			{
				CreateNextFile(track, m_silencestart);
				m_minSilCountDown = m_minTrackLen * m_decoder.GetSampleRate();
			}
		}
//		LogMessage(LOG_DEBUG, "Current Pos: %d\n", 
//				bytesread);
	}
	CreateNextFile(track, bytesread);					// make the last track
	fclose(m_fpIn);
}

//-------------------------------------------------------------------------
// CreateNextFile
//-------------------------------------------------------------------------
void CTrackSplitter::CreateNextFile(const char* fromtrack, long pos)
{
	static long lastpos = 0;
	char newtrack[MAX_PATH];
	sprintf(newtrack, "track-%02d.mp3", m_curFileIndex);
	long oldpos = ftell(m_fpIn);					// remember old pos

	FILE* fp = fopen(newtrack, "wb");
	if (!fp)
		throw CTrackSplitter_CantOpenTrackFile();

	int readremaining = pos-lastpos;
	if (fseek(m_fpIn, lastpos, SEEK_SET) != 0)
		throw CTrackSplitter_InvalidFilePosition();

	if (m_pEventHandler)
		m_pEventHandler->OnNewTrack(newtrack, lastpos, pos);

	char rbuf[0xFFFF];
	int readsize = 0xFFFF;
	while(1)
	{
		long ret = fread(rbuf, sizeof(char), readsize, m_fpIn);
		if (ret == 0)
			break;
		readremaining -= ret;
		if (readremaining < readsize)
			readsize = readremaining;
		fwrite(rbuf, sizeof(char), ret, fp);
	}
	
	fclose(fp);
	lastpos = pos;
	m_curFileIndex++;
	fseek(m_fpIn, oldpos, SEEK_SET);				// go back to old pos
}

//-------------------------------------------------------------------------
// OnMp3Data (event)
//-------------------------------------------------------------------------
void CTrackSplitter::OnMp3Data (const short *samples, int nsamples, int nchannels, int in_samplerate, int bitrate)
{
	static signed short lastSample = 0;
	static signed short sample = 0;
	short const *left_ch, *right_ch;
	left_ch   = &samples[0];
	right_ch  = &samples[1];

	if (m_foundSilence)
		return;

	m_minSilCountDown -= nsamples;
	if (m_minSilCountDown > 0)
		return;

	while(nsamples--)
	{
		/* output sample(s) in 16-bit signed little-endian PCM */
		lastSample = sample;
		sample = (short)*left_ch++;

		if (nchannels == 2) 
		{
			// make mono
			sample = (sample + *right_ch++)/2;
		}

		//
		// get the instantanous volume
		//
		double vol = (lastSample*lastSample)+(sample*sample);
		vol = sqrt(vol / 2);

		if (vol < m_silencevol)
		{
			if (m_insilencecount == 0)
			{
				m_silencestart = m_decoder.GetBytesDecoded();
			}
			m_insilencecount++;
		}
		else
		{
			m_insilencecount = 0;
		}

		if (m_insilencecount > (int)(in_samplerate*m_silencedur))
		{
			m_foundSilence = true;
			return;
		}
	}
}
