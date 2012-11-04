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

#include <errno.h>
#include <string.h>
#include "audiotap_callback.h"
#include "audio2tap_core.h"
#include "audiotap_loop.h"
#include "audiotap.h"

void audio2tap(char **infiles,
               int numinfiles,
               char *outfile,
               uint32_t freq,
               struct tapenc_params *params,
               uint8_t tap_version,
               uint8_t machine,
               uint8_t videotype
)
{
  struct audiotap *audiotap_in, *audiotap_out;
  uint8_t halfwaves = 1;
  uint8_t doing_audio = 0;
  int currentinfile = 0;

  if (tap_version > 2){
    error_message("TAP version %u is unsupported", tap_version);
    return;
  }

  if (tap2audio_open_to_tapfile3(&audiotap_out,
                                 outfile, tap_version,
                                 machine,
                                 videotype) != AUDIOTAP_OK){
    error_message("Cannot open file %s: %s", outfile, strerror(errno));
    return;
  }

  if (numinfiles == 0){
    if (audio2tap_from_soundcard4(&audiotap_in,
                                  freq,
                                  params,
                                  machine,
                                  videotype) != AUDIOTAP_OK){
      error_message("Sound card cannot be opened");
      tap2audio_close(audiotap_out);
      return;
    }
    doing_audio = 1;
  }

  while (doing_audio || currentinfile < numinfiles){
    uint8_t in_machine = machine;
    uint8_t in_videotype = videotype;
    uint8_t retval = 0;

    if (!doing_audio && audio2tap_open_from_file3(&audiotap_in,
                                                  infiles[currentinfile],
                                                  params,
                                                  &in_machine,
                                                  &in_videotype,
                                                  &halfwaves) != AUDIOTAP_OK)
      error_message("File %s does not exist, is not a supported audio file, or cannot be opened for some reasons", infiles);
    else{
      unsigned int datalen;

      tap2audio_enable_halfwaves(audiotap_out, halfwaves && tap_version == 2);
      audio2tap_enable_disable_halfwaves(audiotap_in, halfwaves && tap_version == 2);
      datalen = audiotap_loop(audiotap_in, audiotap_out, audiotap_in, 0, &retval);
      if (halfwaves && tap_version == 2 && (datalen % 2) != 0)
        tap2audio_set_pulse(audiotap_out, 256);
    }
    if (retval != 0 || doing_audio)
      break;
    currentinfile ++;
  }
  tap2audio_close(audiotap_out);
}
