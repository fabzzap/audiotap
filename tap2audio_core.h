/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2005
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audio2tap_core.h : header file for audio2tap_core.c
 * Also implements the callback functions for the core processing part
 * 
 * This file belongs to the tap->audio part
 * This file is part of Audiotap core files
 */

void tap2audio(char *infile,
	      char *outfile,
	      int inverted,
	      int32_t volume,
	      int freq);
void tap2audio_interrupt(void);
