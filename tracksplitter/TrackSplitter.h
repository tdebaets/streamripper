// TrackSplitter1.h: interface for the CTrackSplitter class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRACKSPLITTER1_H__59B8AA99_C098_4AD7_976E_D8BE68A030AD__INCLUDED_)
#define AFX_TRACKSPLITTER1_H__59B8AA99_C098_4AD7_976E_D8BE68A030AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "mp3decoder.h"
#include <assert.h>
#include <math.h>
#include "common.h"

#define MP3_READ_SIZE	MIN_MP3_STREAM_SIZE

#ifndef MAX_PATH
	#define MAX_PATH					260
#endif

#define MIN_RMS_SILENCE				100		// max this configurable
#define MAX_RMS_SILENCE				0x7FFF	// max short
#define NUM_SILTRACKERS				30
#define READSIZE					2000
#define MIN_TRACK_LEN				60
#define DEFAULT_RMS_SILENCE			1500
#define DEFAULT_SILENCE_DURATION	1.0f

DECL_ERROR_DESC(CTrackSplitter_CantOpenFile,		"Can't open file")
DECL_ERROR_DESC(CTrackSplitter_CantOpenTrackFile,	"Can't open track file")
DECL_ERROR_DESC(CTrackSplitter_InvalidFilePosition,	"Invalid file position")

struct ITrackSplitterEvents
{
	virtual void OnNewTrack(const char* trackname, long start, long end) = 0;
	virtual void OnDataRead(long bytepos) = 0;
};

class CTrackSplitter : public IDecodeHandler
{
public:
	CTrackSplitter();

	void			SetEventHandler	(ITrackSplitterEvents& e)	{m_pEventHandler = &e;}
	void			SetSilenceVol	(unsigned short silvol) {m_silencevol = silvol;}
	void			SetSilenceDur	(float sildur)			{m_silencedur = sildur;}
	void			SetMinTrackLen	(long mintracklen)		{m_minTrackLen = mintracklen;}
	unsigned short	GetSilenceVol	()				{return m_silencevol;}
	float			GetSilenceDur	()				{return m_silencedur;}
	long			GetMinTrackLen	()				{return m_minTrackLen;}

	void			SplitTrack		(const char* track);

private:
	virtual void	OnMp3Data		(const short *samples, int nsamples, int nchannels, int in_samplerate, int bitrate);
	void			CreateNextFile	(const char* fromtrack, long pos);

	CTrackSplitter	(const CTrackSplitter&);
	operator=		(const CTrackSplitter&);

	FILE*			m_fpIn;
	CMp3Decoder		m_decoder;
	long			m_curFileIndex;
	double			m_silencevol;
	long			m_insilencecount;
	long			m_silencestart;
	bool			m_foundSilence;
	long			m_minTrackLen;
	long			m_minSilCountDown;
	float			m_silencedur;
	ITrackSplitterEvents*	m_pEventHandler;
};



#endif // !defined(AFX_TRACKSPLITTER1_H__59B8AA99_C098_4AD7_976E_D8BE68A030AD__INCLUDED_)
