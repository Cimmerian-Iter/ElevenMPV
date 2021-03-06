#ifndef _ELEVENMPV_AUDIO_XM_H_
#define _ELEVENMPV_AUDIO_XM_H_

#include <psp2/types.h>

int XM_Init(const char *path);
SceUInt32 XM_GetSampleRate(void);
SceUInt8 XM_GetChannels(void);
void XM_Decode(void *buf, unsigned int length, void *userdata);
SceUInt64 XM_GetPosition(void);
SceUInt64 XM_GetLength(void);
void XM_Term(void);

#endif
