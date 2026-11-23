// Witchaven 360: jfaudiolib's MIDI driver for the console, an OPL3 synthesizer (xenon/driver_opl.c). jfaudiolib's
// driver table (drivers.c) gives these to its SDL entry, which otherwise has no MIDI.

#ifndef DRIVER_OPL_H
#define DRIVER_OPL_H

#include "midifuncs.h"

int  OPLDrv_MIDI_Init(midifuncs *funcs, const char *params);
void OPLDrv_MIDI_Shutdown(void);
int  OPLDrv_MIDI_StartPlayback(void (*service)(void));
void OPLDrv_MIDI_HaltPlayback(void);
void OPLDrv_MIDI_SetTempo(int tempo, int division);
void OPLDrv_MIDI_Lock(void);
void OPLDrv_MIDI_Unlock(void);

#endif
