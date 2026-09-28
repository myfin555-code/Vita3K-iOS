#pragma once
struct EmuEnvState;
// Called on the SDL/UIKit main thread while a game owns the emulator state.
void vita3k_ios_update_keyboard(EmuEnvState &emuenv);
void vita3k_ios_close_keyboard();
