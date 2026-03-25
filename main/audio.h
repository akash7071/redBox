#pragma once

void init_audio(void);
void play_beep(void);
void audio_start_mic_monitor(void (*on_loud_sound)(void));
