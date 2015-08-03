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
               unsigned int numinfiles,
               char *outfile,
               uint32_t freq,
               struct tapenc_params *params,
               uint8_t tap_version,
               uint8_t machine,
               uint8_t videotype
)
{
  struct audiotap *audiotap_in, *audiotap_out = NULL;
  uint8_t halfwaves = 1;
  unsigned int currentinfile = 0;
  uint8_t retval = 0;

  if (tap_version > 2){
    error_message("TAP version %u is unsupported", tap_version);
    return;
  }

  do{
    uint8_t in_machine = machine;
    uint8_t in_videotype = videotype;
    unsigned int datalen;

    if (numinfiles != 0){
      char *input_file = infiles[currentinfile++];
      if (audio2tap_open_from_file3(&audiotap_in,
                                    input_file,
                                    params,
                                    &in_machine,
                                    &in_videotype,
                                    &halfwaves) != AUDIOTAP_OK){
        error_message("File %s does not exist, is not a supported audio file, or cannot be opened for some reasons", input_file);
        continue;
      }
      update_input_filename(input_file);
    }
    else if (audio2tap_from_soundcard4(&audiotap_in,
                                  freq,
                                  params,
                                  machine,
                                  videotype) != AUDIOTAP_OK){
      error_message("Sound card cannot be opened");
      continue;
    }

    if (audiotap_out == NULL && tap2audio_open_to_tapfile3(&audiotap_out,
                                                            outfile, tap_version,
                                                            machine,
                                                            videotype) != AUDIOTAP_OK){
      error_message("Cannot open file %s for writing", outfile);
      audiotap_out = NULL;
      retval = -1;
    }

    if (audiotap_out != NULL){
      tap2audio_enable_halfwaves(audiotap_out, halfwaves && tap_version == 2);
      audio2tap_enable_disable_halfwaves(audiotap_in, halfwaves && tap_version == 2);
      datalen = audiotap_loop(audiotap_in, audiotap_out, audiotap_in, &retval);
      if (halfwaves && tap_version == 2 && (datalen % 2) != 0)
        tap2audio_set_pulse(audiotap_out, 256);
    }
    audio2tap_close(audiotap_in);
  } while(currentinfile != numinfiles && retval == 0);
  if (audiotap_out != NULL)
    tap2audio_close(audiotap_out);
}
