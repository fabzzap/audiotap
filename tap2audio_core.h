/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2005
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * tap2audio_core.h : header file for tap2audio_core.c
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
