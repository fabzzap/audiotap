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

void sig_int(int signum){
	tap2audio_interrupt();
}

int main(int argc, char** argv){
  int inverted = 0;
  int volume = 254;
  int freq = 44100;
  struct option cmdline[]={
    {"volume"            ,1,NULL,'v'},
    {"frequency"         ,1,NULL,'f'},
    {"inverted-waveform" ,0,NULL,'i'},
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

  while( (option=getopt_long(argc,argv,"d:h:0i",cmdline,NULL)) != -1){
    switch(option){
    case 'v':
      if (atoi(optarg) < 1 || atoi(optarg) > 255){
	printf("Volume out of range 1-255\n");
	exit(1);
      };
      volume=atoi(optarg);
      break;
    case 'f':
      freq=atoi(optarg);
      break;
    case 'i':
      inverted=1;
      break;
    default:
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

  tap2audio(argv[0], argv[1], inverted, volume << 23, freq);
  exit(0);
}
