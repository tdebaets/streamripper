#ifndef __PARSE_H__
#define __PARSE_H__

#include "types.h"

void init_metadata_parser (char* rules_file);
void parse_metadata (TRACK_INFO* track_info);

#endif //__PARSE_H__
