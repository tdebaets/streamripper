#include <stdio.h>
#include <conio.h>
#include <direct.h>
#include "types.h"
#include "testcommon.h"
#include "ripman_common.h"

void main()
{
	//
	// remake dir's
	// JCBUG -- make this nicer
	char basedir[] = "/sripper_1x/tinytest/";
	system("rmdir /s /q testdir");
	system("mkdir testdir");
	
	chdir(basedir);
//	UNIT_TEST("OPT_SEPERATE_DIRS",
//			  test_seperate_dirs("http://localhost:8000"))

	chdir(basedir);
//	UNIT_TEST("OPT_OVER_WRITE_TRACKS",
//			  test_over_write_tracks("http://localhost:8000"))

	chdir(basedir);
	UNIT_TEST("OPT_SEARCH_PORTS",	// JCBUG -- this one needs a local scserver on port 8k
			  test_search_ports("http://localhost:8000"))
	

//	UNIT_TEST("OPT_AUTO_RECONNECT",
//			  test_auto_recon("http://localhost:8000"))
	WAIT_FOR_KEY()
}





/*
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

*/