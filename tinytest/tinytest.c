#include <stdio.h>
#include "findsep.h"

#define MSIZE	1000000

void main()
{
	
	FILE *fp = fopen("test.mp3", "rb");
	char *buf = malloc(MSIZE);
	unsigned long pos;
	int size;
		
	size = fread(buf, 1, MSIZE, fp);
	findsep_silence(buf, size, &pos);

	{
		FILE *fp1 = fopen("part1.mp3", "wb");
		fwrite(buf, 1, pos, fp1);
		fclose(fp1);

		fp1 = fopen("part2.mp3", "wb");
		fwrite(buf+pos, 1, size-pos, fp1);
		fclose(fp1);

	}
	free(buf);

}