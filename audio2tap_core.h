/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audio2tap_core.h : header file for audio2tap_core.c
 * Also implements the callback functions for the core processing part
 * 
 * This file belongs to the audio->tap part
 * This file is part of Audiotap core files
 */

#include <stdint.h>

struct tapenc_params;

void audio2tap(char **infiles,
               unsigned int numinfiles,
               char *outfile,
               uint32_t freq,
               struct tapenc_params *params,
               uint8_t tap_version,
               uint8_t machine,
               uint8_t videotype);

