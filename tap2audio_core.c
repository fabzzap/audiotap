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
#include <sys/types.h>
#include <errno.h>
#include "audiotap.h"
#include "audiotap_callback.h"

static const char c64_machine_string[]="C64-TAPE-RAW";
static const char c16_machine_string[]="C16-TAPE-RAW";

struct audiotap *audiotap;

void tap2audio_interrupt(void){
  audiotap_terminate(audiotap);
}

void tap2audio(char *infile,
	      char *outfile,
	      int inverted,
	      int32_t volume,
	      int freq)
{
  FILE *fd;
  char machine_string[12], tap_version, machine, videotype;
  unsigned int datalen = 0, old_datalen_div_10000 = 0;
  int totlen;
  u_int32_t pulse, this_pulse, overflow_pulse;
  unsigned char byte, threebytes[3];
  enum audiotap_status status;
  unsigned char end_of_file = 0;

  fd=fopen(infile, "rb");
  if (fd == NULL){
    error_message("Cannot open file %s: %s", infile, strerror(errno));
    return;
  }

  if (fread(machine_string, 12, 1, fd) != 1){
    error_message("%s does not seem to be a TAP file", infile);
    return;
  }

  if (memcmp(machine_string, c64_machine_string, 12) &&
      memcmp(machine_string, c16_machine_string, 12)){
    error_message("%s does not seem to be a TAP file", infile);
    return;
  }

  if (fread(&tap_version, 1, 1, fd) != 1 ||
      fread(&machine, 1, 1, fd) != 1 ||
      fread(&videotype, 1, 1, fd) != 1){
    error_message("%s does not seem to be a TAP file", infile);
    return;
  }

  if(machine != TAP_MACHINE_C64 &&
     machine != TAP_MACHINE_VIC &&
     machine != TAP_MACHINE_C16){
    error_message("File %s has unsupported machine %u", infile, machine);
    return;
  }
    
  if(videotype != TAP_VIDEOTYPE_PAL &&
     videotype != TAP_VIDEOTYPE_NTSC){
    error_message("File %s has unsupported TAP videotype %u", infile, videotype);
    return;
  }
    
  if(tap_version != 0 && tap_version != 1){
    error_message("File %s has unsupported TAP version %u", infile, tap_version);
    return;
  }
    
  if(fseek(fd, 0, SEEK_END) != 0){
    error_message("Error in fseek: %s", strerror(errno));
    return;
  }

  if ( (totlen = ftell(fd)) == -1){
    error_message("Error in ftell: %s", strerror(errno));
    return;
  }

  if(totlen < 20){
    error_message("%s does not seem to be a TAP file", infile);
    return;
  }

  if(fseek(fd, 20, SEEK_SET) != 0){
    error_message("Error in fseek: %s", strerror(errno));
    return;
  }

  if (tap2audio_open(&audiotap, outfile, volume, freq, inverted, machine, videotype) != AUDIOTAP_OK){
    if (outfile)
      error_message("File %s cannot be opened", infile);
    else
      error_message("Sound card cannot be opened", infile);
    return;
  }

  statusbar_initialize(totlen);
  datalen = 20;
  overflow_pulse = tap_version == 0 ? 256*8 : 0xFFFFFF;

  status = AUDIOTAP_OK;
  while(1){
    if (datalen/10000 > old_datalen_div_10000){
      old_datalen_div_10000 = datalen/10000;
      statusbar_update(datalen);
    }

    pulse = 0;
    do{
      if (fread(&byte, 1, 1, fd) != 1){
        end_of_file = 1;
        break;
      }
      datalen++;
      if (byte != 0)
        this_pulse = byte * 8;
      else if (tap_version == 0)
        this_pulse = 256 * 8;
      else{
        if (fread(threebytes, 3, 1, fd) != 1){
          end_of_file = 1;
          break;
        }
        datalen += 3;
        this_pulse = (threebytes[0]      ) +
	                   (threebytes[1] <<  8) +
	                   (threebytes[2] << 16);
      }
      pulse += this_pulse;
    }while(this_pulse == overflow_pulse);
    if (end_of_file)
    {
      break;
    }
    status = tap2audio_set_pulse(audiotap, pulse);
    if (status != AUDIOTAP_OK) break;
  }
  statusbar_update(datalen);
  statusbar_exit();
  if (status == AUDIOTAP_INTERRUPTED)
    warning_message("Interrupted");
  else if(status != AUDIOTAP_OK)
    error_message("Something went wrong");
 
  fclose(fd);
  tap2audio_close(audiotap);
}
