#ifndef __GETOPT_H__
#define __GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int getopt (int argc, char *argv[], char *optstring);
extern int		optind;
extern char*	optarg;
extern int		opterr;

#ifdef __cplusplus
}
#endif


#endif //__GETOPT_H__