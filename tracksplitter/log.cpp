#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "log.h"

static	int	globalErrorType = LM_INFO;
static	char	gLogFile[1024] = "";

void setErrorType(int type) {
	globalErrorType = type;
}

void setLogFile(char *file) {
	if (file) {
		strcpy(gLogFile, file);
	}
}
void LogMessage(int type, char *source, int line, char *fmt, ...) {
	va_list parms;
	char	errortype[25] = "";
	int	addNewline = 1;
	static	FILE	*filep = stdout;
	struct tm *tp;
	long t;
	int parseableOutput = 0;
	char    timeStamp[255];

	memset(timeStamp, '\000', sizeof(timeStamp));

	time(&t);
	tp = localtime(&t);
	strftime(timeStamp, sizeof(timeStamp), "%m/%d/%y %T", tp);

	switch (type) {
		case LM_ERROR:
			strcpy(errortype, "Error");
			break;
		case LM_INFO:
			strcpy(errortype, "Info");
			break;
		case LM_DEBUG:
			strcpy(errortype, "Debug");
			break;
		default:
			strcpy(errortype, "Unknown");
			break;
	}

	if (fmt[strlen(fmt)-1] == '\n') {
		addNewline = 0;
	}


	if (type <= globalErrorType) {
		va_start(parms, fmt);

		if (gLogFile[0] != '\0') {
			filep = fopen(gLogFile, "a");
		}
		
		fprintf(filep,  "%s: ", errortype);
		vfprintf(filep, fmt, parms);
		va_end(parms);
		if (addNewline) {
			fprintf(filep, "\n");
		}
		fflush(filep);
	}

}
