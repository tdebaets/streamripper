#include <stdio.h>
#include "findsep.h"

#define MSIZE	1000000
#include "rip_manager.h"
#include "types.h"
#include <conio.h>
#include "socklib.h"
#include "inet.h"
#include <process.h>

void handle_RM_UPDATE(RIP_MANAGER_INFO *info)
{
	static char *statusstr[] = {"***ERROR***", 
								"RM_STATUS_BUFFERING", 
								"RM_STATUS_RIPPING", 
								"RM_STATUS_RECONNECTING"};

	printf("*got RM_UPDATE\n");
	printf("streamname: %s\n"
		   "server_name: %s\n"
		   "bitrate: %d\n"
		   "meta_interval: %d\n"
		   "filename: %s\n"
		   "filesize: %d\n"
		   "status: %s\n", 
			info->streamname,
			info->server_name,
			info->bitrate,
			info->meta_interval,
			info->filename,
			info->filesize,
			statusstr[info->status]);

	if (info->status == 0)
		printf("HOLD ON\n");
}

void handle_RM_ERROR(void *arg)
{
	ERROR_INFO *err = (ERROR_INFO*)arg;
	printf("*got RM_ERROR\n"
		   "error: %d\n"
		   "info: %s\n", 
		    err->error_code,
			err->error_str);
}

void handle_RM_NEW_TRACK(char *track)
{
	printf("*got RM_NEW_TRACK\n"
		   "track: %s\n",
		   track);
		  
}

void handle_RM_TRACK_DONE(char *track)
{
	printf("*got RM_TRACK_DONE\n"
		   "track: %s\n",
		   track);
		  
}

void handle_RM_OUTPUT_DIR(char *output_dir)
{
	printf("*got RM_OUTPUT_DIR\n"
		   "output_dir: %s\n",
		   output_dir);
		  
}

void rip_manager_proc(int msg, void *arg)
{
	switch(msg)
	{
	case RM_UPDATE:		// returns a pointer RIP_MANAGER_INFO struct
		handle_RM_UPDATE(arg);
		break;
	case RM_ERROR:		// returns the error code
		handle_RM_ERROR(arg);
		break;
	case RM_DONE:		// NULL
		printf("*got RM_DONE\n");
		break;
	case RM_STARTED:	// NULL
		printf("*got RM_STARTED\n");
		break;
	case RM_NEW_TRACK:	// Name of the new track
		handle_RM_NEW_TRACK(arg);
		break;
	case RM_TRACK_DONE:	// Name of the track completed
		handle_RM_TRACK_DONE(arg);
		break;
	case RM_OUTPUT_DIR:	// Full path of the output directory
		handle_RM_OUTPUT_DIR(arg);
		break;
	default:
		printf("Unexpected msg!: %d\n", msg);
	}

}


void test_hammer(char *url)
{
	int i;
	RIP_MANAGER_OPTIONS opt;

	printf("Testing streamripper plain\n");
	memset(&opt, 0, sizeof(RIP_MANAGER_OPTIONS));

	opt.relay_port = 8000;
	opt.max_port = 8000;
	strcpy(opt.url, url);
	
	printf("Starting and stopping 1000 times\n");
	for(i = 0; i < 1000; i++)
	{
		rip_manager_start(rip_manager_proc, &opt);
		rip_manager_stop();
	}
}


void connect_to_relay_thread(void *url)
{
	HSOCKET sock;
	SR_HTTP_HEADER info;
	int ret;

	printf("connecting to relay: %s\n", url);
	ret = inet_sc_connect(&sock, (char*)url, NULL, &info);
	if (ret != SR_SUCCESS)
	{
		printf("inet_sc_connect FAILED!\n",
			   "error: %d\n"
			   "str: %s\n",
			   ret,
				rip_manager_get_error_str(ret));
		return;
	}

	{
		char buf[0x400];
		while(1)
		{
			ret = socklib_recvall(&sock, buf, 0x400);
			printf("recv: %d bytes\n", ret);
			if (ret < 0)
			{
				printf("socklib_recvall: %d\n", WSAGetLastError());
				break;
			}
		}
	}
	socklib_close(&sock);
	printf("done with relay: %s\n", url);

}

void connect_to_relay(void *url)
{
	_beginthread(connect_to_relay_thread, 0, url);
}

void test_relay(char *url)
{
	int i;
	RIP_MANAGER_OPTIONS opt;
	float stime;

	printf("Testing streamripper plain\n");
	memset(&opt, 0, sizeof(RIP_MANAGER_OPTIONS));

	opt.relay_port = 8000;
	opt.max_port = 9000;
	strcpy(opt.url, url);
	
	printf("Starting and stopping 100 times\n");
	for(i = 0; i < 100; i++)
	{
		stime = ((rand() / (float)RAND_MAX) * 60 + 1);
		rip_manager_start(rip_manager_proc, &opt);
		connect_to_relay("http://localhost:8000");
		Sleep(10000);	// 1 - 2 min
		rip_manager_stop();
	}
}

void test_relay_simple(char *url)
{
	int i;
	RIP_MANAGER_OPTIONS opt;

	printf("Testing streamripper plain\n");
	memset(&opt, 0, sizeof(RIP_MANAGER_OPTIONS));

	opt.relay_port = 9000;
	opt.max_port = 9000;
	strcpy(opt.url, url);
	
	printf("Starting and stopping 100 times\n");
	for(i = 0; i < 100; i++)
	{
		rip_manager_start(rip_manager_proc, &opt);
		rip_manager_stop();
	}
}

void test_auto_reconnect(char *url)
{
	RIP_MANAGER_OPTIONS opt;

	printf("Testing streamripper plain\n");
	memset(&opt, 0, sizeof(RIP_MANAGER_OPTIONS));

	opt.flags = OPT_AUTO_RECONNECT;
	opt.relay_port = 9000;
	opt.max_port = 9000;
	strcpy(opt.url, url);
	printf("Starting, go stop the shoutcast server when ready\n");
	rip_manager_start(rip_manager_proc, &opt);

	getch();
	rip_manager_stop();

}

void test_plain(char *url)
{
	RIP_MANAGER_OPTIONS opt;

	printf("Testing streamripper plain\n");
	memset(&opt, 0, sizeof(RIP_MANAGER_OPTIONS));

	opt.flags = OPT_NO_RELAY;
	strcpy(opt.url, url);
	
	printf("Starting...\n");
	rip_manager_start(rip_manager_proc, &opt);
	getch();
	rip_manager_stop();
}


void test_overnight(char *url)
{
	RIP_MANAGER_OPTIONS opt;
	int i = 0;
	long stime = 0;

	printf("Testing streamripper overnight\n");
	memset(&opt, 0, sizeof(RIP_MANAGER_OPTIONS));

	strcpy(opt.url, url);
	strcpy(opt.output_directory, ".");
	opt.flags = OPT_AUTO_RECONNECT;
	opt.relay_port = 8050;

	srand(1);
	for(i = 0; i < 100; i++)
	{
		stime = (long) ((rand() / (float)RAND_MAX) * 60 + 1);
		printf("Rip %d for %d min...\n", i, stime);
		rip_manager_start(rip_manager_proc, &opt);
		Sleep(stime*60000);	// 1 - 60 min
		rip_manager_stop();
	}
}



void main()
{


	test_overnight("http://205.188.245.132:8038");
	test_auto_reconnect("http://maris:8000");
	test_plain("http://maris:8000");
	test_hammer("http://maris:8000");
	test_relay("http://maris:8000");

//	printf("Done, press any key to quit.\n");
//	getch();
}