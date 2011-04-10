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

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include "audiotap.h"
#include "audiotap_callback.h"
#include "tap2audio_core.h"

static const char c64_machine_string[]="C64-TAPE-RAW";
static const char c16_machine_string[]="C16-TAPE-RAW";

struct audiotap *audiotap = NULL;

void tap2audio_interrupt(int ignored){
  audiotap_terminate(audiotap);
}

void tap2audio(char *infile,
	      char *outfile,
	      uint8_t inverted,
	      enum tapdec_waveform waveform,
	      int32_t volume,
	      int freq)
{
  uint8_t semiwaves, machine, videotype;
  unsigned int datalen = 0;
  int totlen;
  enum audiotap_status status = AUDIOTAP_OK;
  enum tap_trigger trigger_type;
  struct audiotap *audiotap_in;
  int currlen;

  if (audio2tap_open_from_file(&audiotap_in,
                               infile,
                               NULL,
                               &machine,
                               &videotype,
                               &semiwaves) != AUDIOTAP_OK){
    error_message("File %s cannot be opened for reading", infile);
    return;
  }
  if (outfile){
    if (tap2audio_open_to_wavfile(&audiotap, outfile, volume, freq, trigger_type, waveform, machine, videotype) != AUDIOTAP_OK){
      error_message("File %s cannot be opened", infile);
      audio2tap_close(audiotap_in);
      return;
    }
  }
  else if(tap2audio_open_to_soundcard(&audiotap, volume, freq, trigger_type, waveform, machine, videotype)){
    error_message("Sound card cannot be opened", infile);
    audio2tap_close(audiotap_in);
    return;
  }

  trigger_type = semiwaves ? TAP_TRIGGER_ON_BOTH_EDGES :
                 inverted  ? TAP_TRIGGER_ON_FALLING_EDGE :
                             TAP_TRIGGER_ON_RISING_EDGE;
  signal(SIGINT, tap2audio_interrupt);
  totlen = audio2tap_get_total_len(audiotap_in);
  statusbar_initialize(totlen);

  while(status == AUDIOTAP_OK){
    uint32_t pulse, raw_pulse;

    if ((datalen++) % 10000 == 9999){
      currlen = audio2tap_get_current_pos(audiotap_in);
      statusbar_update(datalen);
    }
    status = audio2tap_get_pulses(audiotap_in, &pulse, &raw_pulse);
    if (status != AUDIOTAP_OK)
      break;

    status = tap2audio_set_pulse(audiotap, pulse);
  }
  statusbar_update(datalen);
  statusbar_exit();
  if (status == AUDIOTAP_INTERRUPTED)
    warning_message("Interrupted");
  else if(status != AUDIOTAP_OK && status != AUDIOTAP_EOF)
    error_message("Something went wrong");
 
  tap2audio_close(audiotap);
  audio2tap_close(audiotap_in);
}

