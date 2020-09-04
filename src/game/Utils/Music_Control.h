#ifndef _MUSIC_CONTROL_H_
#define _MUSIC_CONTROL_H_

#include <string_theory/string>

#include "SoundMan.h"
#include "ContentMusic.h"

extern MusicMode gubMusicMode;
extern BOOLEAN	gfForceMusicToTense;


void SetMusicMode(MusicMode ubMusicMode);

/* Starts up one of the tunes in the music list. */
void MusicPlay(const ST::string* pFilename);

void MusicSetVolume(UINT32 uiVolume);
UINT32 MusicGetVolume(void);

/* Handles any maintenance the music system needs done. Should be polled from
 * the main loop, or somewhere with a high frequency of calls. */
void MusicPoll(void);

void SetMusicFadeSpeed( INT8 bFadeSpeed );

#endif
