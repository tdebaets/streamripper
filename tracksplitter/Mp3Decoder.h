// Mp3Decoder.h: interface for the CMp3Decoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MP3DECODER_H__B1E9CC74_B2DC_4D99_8471_50EE316640CC__INCLUDED_)
#define AFX_MP3DECODER_H__B1E9CC74_B2DC_4D99_8471_50EE316640CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "mad.h"
#include "common.h"

#define MP3BUFSIZE (0x100000)	// 1M how much space we can hold
#define ERR_MP3DECODER_BUFFER_FULL -1
#define MIN_MP3_STREAM_SIZE	(0x10000) // 64k - how much we should process at a time

struct IDecodeHandler
{
	virtual void OnMp3Data		(const signed short *samples, int nsamples, int nchannels, int in_samplerate, int bitrate) = 0;
};

DECL_ERROR(CMp3Decoder_BufferFull)

class CMp3Decoder
{
public:
	CMp3Decoder					();
	virtual ~CMp3Decoder		();

	void	SetDecodeHandler	(IDecodeHandler& decoder)	{m_pdecodeHandler = &decoder;}
	void	FeedStream			(char *pdata, long size) throw(CMp3Decoder_BufferFull);
	int		Decode				();
	long	GetStreamSize		();

	long	GetBitRate			()							{return m_bitRate;}
	long	GetSampleRate		()							{return m_sampleRate;}
	long	GetNumChannels		()							{return m_nch;}
	long	GetBytesDecoded		()							{return m_bytesDecoded;}
private:
	void	HandleMp3Output		();

	// prevent copies
	CMp3Decoder& operator=	(CMp3Decoder&);
	CMp3Decoder				(const CMp3Decoder&);

	long				m_bytesDecoded;
	char*				m_pmp3Buf;
	long				m_mp3BufPos;
	struct mad_stream	m_stream;
	struct mad_frame	m_frame;
	struct mad_synth	m_synth;
	int					m_sampleRate;
	long				m_bitRate;
	int					m_nch;					// number of channels
	IDecodeHandler*		m_pdecodeHandler;
};

#endif // !defined(AFX_MP3DECODER_H__B1E9CC74_B2DC_4D99_8471_50EE316640CC__INCLUDED_)
