/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audio2tap_core.c : main audio->tap processing
 *
 * This file belongs to the audio->tap part
 * This file is part of Audiotap core processing files
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include "audiotap_callback.h"
#include "audio2tap_core.h"

static const char c64_machine_string[]="C64-TAPE-RAW";
static const char c16_machine_string[]="C16-TAPE-RAW";

static struct audiotap *audiotap_in = NULL;

void audio2tap_interrupt(int ignored)
{
  audiotap_terminate(audiotap_in);
}

void audio2tap(char *infile,
          char *outfile,
          uint32_t freq,
          struct tapdec_params *params,
          unsigned char tap_version,
          int clock,
          uint8_t videotype
)
{
  FILE *fd;
  enum audiotap_status ret = AUDIOTAP_OK;
  const char *machine_string;
  char buffer[4];
  unsigned int datalen = 0, old_datalen_div_10000 = 0;
  int totlen, currlen;
  int32_t currloudness;
  uint8_t machine;
  struct audiotap *audiotap_out;

  if (tap_version > 1){
    error_message("TAP version %u is unsupported", infile);
    return;
  }

  switch(clock){
  default:
    machine=TAP_MACHINE_C64;
    break;
  case 1:
    machine=TAP_MACHINE_VIC;
    break;
  case 2:
    machine=TAP_MACHINE_C16;
    break;
  case 3:
    machine=TAP_MACHINE_C16;
    params->inverted = TAP_TRIGGER_ON_BOTH_EDGES;
    tap_version = 2;
    break;
  }

  if (infile){
    if(audio2tap_open_from_file(&audiotap_in,
                                infile,
                                params,
                                machine,
                                videotype) != AUDIOTAP_OK){
      error_message("File %s does not exist, is not a supported audio file, or cannot be opened for some reasons", infile);
      return;
    }
  }
  else if (audio2tap_from_soundcard(&audiotap_in,
                                    freq,
                                    params,
                                    machine,
                                    videotype) != AUDIOTAP_OK){
    error_message("Sound card cannot be opened");
    return;
  }

  if (tap2audio_open_to_tapfile(&audiotap_out, outfile, tap_version,
                                machine,
                                videotype) != AUDIOTAP_OK){
    error_message("Cannot open file %s: %s", outfile, strerror(errno));
    audio2tap_close(audiotap_in);
    return;
  }

  if ( (totlen = audio2tap_get_total_len(audiotap_in)) != -1)
    statusbar_initialize(totlen);
  else
    statusbar_initialize(INT_MAX);

  signal(SIGINT, audio2tap_interrupt);

  while(ret == AUDIOTAP_OK){
    uint32_t pulse, raw_pulse;
    if (datalen/10000 > old_datalen_div_10000){
      old_datalen_div_10000 = datalen/10000;
      currlen = audio2tap_get_current_pos(audiotap_in);
      if (currlen != -1)
        statusbar_update(currlen);
      else{
        currloudness=audio2tap_get_current_sound_level(audiotap_in);
        if (currloudness != -1)
          statusbar_update(currloudness);
      }
    }
    ret=audio2tap_get_pulses(audiotap_in, &pulse, &raw_pulse);
    if (ret!=AUDIOTAP_OK)
      break;
    ret=tap2audio_set_pulse(audiotap_out, pulse);
  }
    
  currlen = audio2tap_get_current_pos(audiotap_in);
  if (currlen != -1)
    statusbar_update(currlen);
  if (totlen  != -1)
    statusbar_exit();

  if(ret==AUDIOTAP_INTERRUPTED)
    warning_message("Interrupted by user");
  else if(ret!=AUDIOTAP_EOF)
    error_message("Something went wrong");

  signal(SIGINT, SIG_DFL);

  audio2tap_close(audiotap_in);
  tap2audio_close(audiotap_out);
}

