// tracksplitter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TrackSplitter.h"
#include <stdlib.h>
#include "getopt.h"
#include <string.h>

void usage()
{
	printf	(	"Usage: tracksplitter [OPTION]... FILE\n"
				"\n"
				" -v	Silence Volume (0 .. %d)\n"
				" -d	Silence Duration (float)\n"
				"\n"
				"FILE must be a valid path to a mp3 file\n"
				"Report bugs to <jonclegg@yahoo.com>\n",
				MAX_RMS_SILENCE
			);
}

#define DEFAULT_VOLUME			2500
#define DEFAULT_DURATION		1
#define DEFAULT_MIN_TRACK_LEN	60

int main(int argc, char* argv[])
{
	CTrackSplitter	ts;
	char			c;
	int				tempint;
	float			tempfloat;

	// defaults
	ts.SetSilenceVol(DEFAULT_VOLUME);
	ts.SetSilenceDur(DEFAULT_DURATION);					
	ts.SetMinTrackLen(DEFAULT_MIN_TRACK_LEN);

	while ((c = getopt(argc, argv, "v:d:l::")) != EOF)
	{
		switch (c)
		{
			case 'v':						// silence volume
				tempint = atoi(optarg);
				if (tempint < 0 || tempint > MAX_RMS_SILENCE)
				{
					fprintf(stderr, 
						"Error: Silence volume can not exceed %d or be below zero\n", 
						MAX_RMS_SILENCE);
					exit(-2);
				}
				ts.SetSilenceVol(tempint);
				break;

			case 'd':						// silence duration
				tempfloat = atof(optarg);
				ts.SetSilenceVol(tempfloat);
				break;

			case 'l':						// min track len
				tempint = atof(optarg);
				ts.SetMinTrackLen(tempint);
				break;
			case '?':
				usage();
				exit(-2);
			default:
				break;
		}
	}
	if (optarg == NULL || 
		strcmp(optarg, "--help") == 0)
	{
		usage();
		exit(-2);
	}

	printf	(
				"Splitting Track:\t%s\n"
				"Silence Volume:\t\t%d\n"
				"Silence Duration:\t%f\n"
				"Min Track Length:\t%d\n\n",
				optarg,
				ts.GetSilenceVol(),
				ts.GetSilenceDur(),
				ts.GetMinTrackLen()
			);
		   
	try
	{
		ts.SplitTrack(optarg);
	}
	catch(std::exception &e)
	{
		printf("Error: %s\n", e.what());
	}
	return 0;
}

