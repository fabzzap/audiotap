/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2011
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audiotap_loop.h : core of both adio->tap and tap->audio
 * 
 * This file is shared between the audio->tap part and the tap->audio part
 * This file is part of Audiotap core processing files
 */

#include <stdint.h>

struct audiotap;

void audiotap_interrupt();
void audiotap_pause();
void audiotap_resume();
unsigned int audiotap_loop(struct audiotap *audiotap_in
                          ,struct audiotap *audiotap_out
                          ,struct audiotap *interruptible
                          ,uint8_t *problems_occurred);
