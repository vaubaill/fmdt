#include <nrc2.h>

#include "ffmpeg-io/reader.h"
#include "ffmpeg-io/writer.h"

#ifndef _VIDEO_HELPER_
#define _VIDEO_HELPER_
typedef struct {
    ffmpeg_handle ffmpeg;
    int frame_start;
    int frame_end;
    int frame_skip;
    int frame_current;
} Video;

Video* Video_init_from_file(char* filename, int start, int end, int skip, int* i0, int* i1, int* j0, int* j1);
int    Video_nextFrame(Video* video, uint8** I);
void   Video_free(Video* video);

#endif // _VIDEO_HELPER_
