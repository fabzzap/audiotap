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
#include <sys/types.h>
#include "audiotap_callback.h"
#include "audiotap.h"

static const char c64_machine_string[]="C64-TAPE-RAW";
static const char c16_machine_string[]="C16-TAPE-RAW";

struct audiotap *audiotap;

void audio2tap_interrupt()
{
  audiotap_terminate(audiotap);
}

void audio2tap(char *infile,
	      char *outfile,
	      u_int32_t freq,
	      u_int32_t min_duration,
	      u_int32_t min_height,
	      int inverted,
	      unsigned char tap_version)
{
  FILE *fd;
  enum audiotap_status ret;
  struct tap_pulse pulse;
  const char *machine_string;
  char buffer[4];
  unsigned int datalen = 0, old_datalen_div_10000 = 0;
  int totlen, currlen;
  int32_t currloudness;

  if (tap_version > 1){
    error_message("TAP version %u is unsupported", infile);
    return;
  }

  if(audio2tap_open(&audiotap, infile, freq, min_duration, min_height, inverted) != AUDIOTAP_OK){
	  if (infile)
		  error_message("File %s does not exist, is not a supported audio file, or cannot be opened for some reasons", infile);
	  else
		  error_message("Sound card cannot be opened");
    return;
  }

  machine_string=c64_machine_string;

  fd=fopen(outfile, "wb");
  if (fd == NULL){
    error_message("Cannot open file %s: %s", outfile, strerror(errno));
    audio2tap_close(audiotap);
    return;
  }

  if (fwrite(machine_string, 12, 1, fd) != 1){
    error_message("Cannot write to file %s: %s", outfile, strerror(errno));
    goto err;
  }

  buffer[0]=tap_version;
  buffer[1]=buffer[2]=buffer[3]=0;
  if (fwrite(buffer, 4, 1, fd) != 1){
    error_message("Cannot write to file %s: %s", outfile, strerror(errno));
    goto err;
  }

  buffer[0]=buffer[1]=buffer[2]=buffer[3]=0;
  if (fwrite(buffer, 4, 1, fd) != 1){
    error_message("Cannot write to file %s: %s", outfile, strerror(errno));
    goto err;
  }

  datalen=0;

  if ( (totlen = audio2tap_get_total_len(audiotap)) != -1)
    statusbar_initialize(totlen);
  else
	  statusbar_initialize(2147483647);

  while(1){
    if (datalen/10000 > old_datalen_div_10000){
      old_datalen_div_10000 = datalen/10000;
      currlen = audio2tap_get_current_pos(audiotap);
      if (currlen != -1)
		  statusbar_update(currlen);
	  else{
		  currloudness=audio2tap_get_current_sound_level(audiotap);
		  if (currloudness != -1)
			  statusbar_update(currloudness);
	  }
    }
    ret=audio2tap_get_pulse(audiotap, &pulse);
    if (ret!=AUDIOTAP_OK) break;

    if (pulse.version_0!=0 || tap_version == 0){
      if (fwrite(&pulse.version_0, 1, 1, fd) != 1){
	error_message("Cannot write to file %s: %s", outfile, strerror(errno));
	goto err;
      }
      datalen+=1;
    }
    else{
      if (fwrite(&pulse.version_1, 4, 1, fd) != 1){
	error_message("Cannot write to file %s: %s", outfile, strerror(errno));
	goto err;
      }
      datalen+=4;
    }
  }
  currlen = audio2tap_get_current_pos(audiotap);
  if (currlen != -1)
    statusbar_update(currlen);
  if (totlen  != -1)
    statusbar_exit();

  if(ret==AUDIOTAP_INTERRUPTED)
    warning_message("Interrupted by user");
  else if(ret!=AUDIOTAP_EOF)
    error_message("Something went wrong");
  if(fseek(fd, 16, SEEK_SET) == -1)
    warning_message("Cannot seek in file %s, len field will be incorrect");
  else{
    buffer[0]= datalen    &0xff;
    buffer[1]=(datalen>> 8)&0xff;
    buffer[2]=(datalen>>16)&0xff;
    buffer[3]=(datalen>>24)&0xff;
    if (fwrite(buffer, 4, 1, fd) != 1){
      warning_message("Cannot write to file %s: %s", outfile, strerror(errno));
    }
  }
 err:
  fclose(fd);
  audio2tap_close(audiotap);
}
