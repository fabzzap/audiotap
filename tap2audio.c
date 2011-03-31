/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * tap2audio.c : main tap->audio program for the command-line version
 * 
 * This file belongs to the tap->audio part
 * This file is part of the command-line vesion of Audiotap
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>

#include "audiotap.h"
#include "tap2audio_core.h"

void sig_int(int signum){
	tap2audio_interrupt();
}

void help(){
  printf("Usage: tap2audio -h|-V\n");
  printf("       tap2audio [-f <freq>] [-v vol] [-i] <input TAP file> [output WAV file]\n");
  printf("Options:\n");
  printf("\t-h: show this help message and exit successfully\n");
  printf("\t-V: show version and copyright info and exit successfully\n");
  printf("\t-i: use inverted waveforms (ignored is input is TAP version 2)\n");
  printf("\t-f <freq>: use output frequency <freq> Hz (default:44100)\n");
  printf("\t-v: volume of the output sound (0-255, default:254)\n");
  printf("\t-w <square|triangle|sine>: set waveform (default:square)\n");
  printf("If no output WAV file is specified, output is sound card\n");
}

void version(){
  printf("tap2audio (part of Audiotap) version 1.4\n");
  printf("(C) by Fabrizio Gennari, 2003-2008\n");
  printf("This program is distributed under the GNU General Public License\n");
  printf("Read the file LICENSE.TXT for details\n");
  printf("This product includes software developed by the NetBSD\n");
  printf("Foundation, Inc. and its contributors\n");
}
   
int main(int argc, char** argv){
  uint8_t inverted = 0;
  enum tapdec_waveform waveform = TAPDEC_SQUARE;
  int volume = 254;
  int freq = 44100;
  struct option cmdline[]={
    {"volume"            ,1,NULL,'v'},
    {"frequency"         ,1,NULL,'f'},
    {"inverted-waveform" ,0,NULL,'i'},
    {"waveform"          ,1,NULL,'w'},
    {"help"              ,0,NULL,'h'},
    {"version"           ,0,NULL,'V'},
    {NULL                ,0,NULL,0}
  };
  int option;
  struct audiotap_init_status status;

  status = audiotap_initialize();
  if (status.audiofile_init_status != LIBRARY_OK &&
      status.pablio_init_status != LIBRARY_OK){
    printf("Failed to initialize audiotap library: both audiofile and pablio failed to load");
    exit(1);
  }

  while( (option=getopt_long(argc,argv,"v:f:ihVw:",cmdline,NULL)) != -1){
    switch(option){
    case 'v':
      volume=atoi(optarg);
      if (volume < 1 || volume > 255){
        printf("Volume out of range 1-255\n");
        exit(1);
      };
      break;
    case 'f':
      freq=atoi(optarg);
      break;
    case 'i':
      inverted=1;
      break;
    case 'h':
      help();
      exit(0);
    case 'V':
      version();
      exit(0);
    case 'w':
      if(!strcmp(optarg,"square"))
        waveform = TAPDEC_SQUARE;
      else if(!strcmp(optarg,"sine"))
        waveform = TAPDEC_SINE;
      else if(!strcmp(optarg,"triangle"))
        waveform = TAPDEC_TRIANGLE;
      else{
        printf("Wrong argument to option -c\n");
        exit(1);
      }
      break;
    default:
      help();
      exit(1);
    }
  }

  argc-=optind;
  argv+=optind;

  if(argc < 1){
    printf("You must specify an input TAP file!\n");
    exit(1);
  }

  if(argc > 2){
    printf("Too many arguments\n");
    exit(1);
  }

  if (argc == 1){
    if (status.pablio_init_status != LIBRARY_OK){
      printf("Cannot read from sound card: pablio library missing or invalid\n");
      exit(1);
    }
  }
  else{
    if (status.audiofile_init_status != LIBRARY_OK){
      printf("Cannot read from file: audiofile library missing or invalid\n");
      exit(1);
    }
  }

  signal(SIGINT, sig_int);

  tap2audio(argv[0], argv[1], inverted, waveform, volume << 23, freq);
  exit(0);
}

