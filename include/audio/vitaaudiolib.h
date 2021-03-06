#ifndef _ELEVENMPV_AUDIO_LIB_H_
#define _ELEVENMPV_AUDIO_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <psp2/audioout.h>

#define VITA_NUM_AUDIO_CHANNELS 1
#define VITA_NUM_AUDIO_SAMPLES 1024
#define VITA_VOLUME_MAX 0x8000

typedef void (* vitaAudioCallback_t)(void *buf, unsigned int reqn, void *pdata);

typedef struct {
	int threadhandle;
	int handle;
	int volumeleft;
	int volumeright;
	vitaAudioCallback_t callback;
	void *pdata;
} VITA_audio_channelinfo;

typedef int (* vitaAudioThreadfunc_t)(int args, void *argp);

void vitaAudioSetVolume(int channel, int left, int right);
int vitaAudioSetFrequency(int channel, int frequency);
void vitaAudioChannelThreadCallback(int channel, void *buf, unsigned int reqn);
void vitaAudioSetChannelCallback(int channel, vitaAudioCallback_t callback, void *pdata);
int vitaAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, void *buf);
int vitaAudioInit(int frequency, SceAudioOutMode mode);
void vitaAudioEndPre(void);
void vitaAudioEnd(void);

#ifdef __cplusplus
}
#endif

#endif
