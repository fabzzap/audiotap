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

void help(){
  printf("Usage: tap2audio -h|-V\n");
  printf("       tap2audio [-f <freq>] [-v vol] [-i] <input TAP file> [output WAV file]\n");
  printf("Options:\n");
  printf("\t-h: show this help message and exit successfully\n");
  printf("\t-V: show version and copyright info and exit successfully\n");
  printf("\t-i use inverted waveforms\n");
  printf("\t-f use output frequency <freq> Hz, default 44100\n");
  printf("\t-v volume of the output sound (0-255, default 254)\n");
  printf("If no output WAV file is specified, output is sound card\n");
}

void version(){
  printf("tap2audio (part of Audiotap) version 1.3\n");
  printf("(C) by Fabrizio Gennari, 2003\n");
  printf("This program is distributed under the GNU General Public License\n");
  printf("Read the file LICENSE.TXT for details\n");
  printf("This product includes software developed by the NetBSD\n");
  printf("Foundation, Inc. and its contributors\n");
}
   
int main(int argc, char** argv){
  int inverted = 0;
  int volume = 254;
  int freq = 44100;
  struct option cmdline[]={
    {"volume"            ,1,NULL,'v'},
    {"frequency"         ,1,NULL,'f'},
    {"inverted-waveform" ,0,NULL,'i'},
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

  while( (option=getopt_long(argc,argv,"v:f:ihV",cmdline,NULL)) != -1){
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
    case 'h':
      help();
      exit(0);
    case 'V':
      version();
      exit(0);
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

  tap2audio(argv[0], argv[1], inverted, volume << 23, freq);
  exit(0);
}
