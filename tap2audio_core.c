/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * tap2audio_core.c : main tap->audio processing
 * 
 * This file belongs to the tap->audio part
 * This file is part of Audiotap core processing files
 */

#include "audiotap_loop.h"
#include "audiotap.h"
#include "audiotap_callback.h"
#include "tap2audio_core.h"

void tap2audio(char *infile,
	      char *outfile,
	      enum tap_trigger inverted,
	      enum tapdec_waveform waveform,
	      int32_t volume,
	      int freq)
{
  uint8_t semiwaves, machine, videotype;
  int totlen;
  struct audiotap *audiotap_in, *audiotap_out;

  if (audio2tap_open_from_file(&audiotap_in,
                               infile,
                               NULL,
                               &machine,
                               &videotype,
                               &semiwaves) != AUDIOTAP_OK){
    error_message("File %s cannot be opened for reading", infile);
    return;
  }
  if (semiwaves)
    inverted = TAP_TRIGGER_ON_BOTH_EDGES;
  if (outfile){
    if (tap2audio_open_to_wavfile(&audiotap_out, outfile, volume, freq, inverted, waveform, machine, videotype) != AUDIOTAP_OK){
      error_message("File %s cannot be opened", infile);
      audio2tap_close(audiotap_in);
      return;
    }
  }
  else if(tap2audio_open_to_soundcard(&audiotap_out, volume, freq, inverted, waveform, machine, videotype)){
    error_message("Sound card cannot be opened", infile);
    audio2tap_close(audiotap_in);
    return;
  }

  totlen = audio2tap_get_total_len(audiotap_in);
  statusbar_initialize(totlen);
  audiotap_loop(audiotap_in, audiotap_out, audiotap_out);
}
