/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audio2tap.c : main audio->tap program for the command-line version
 * 
 * This file belongs to the audio->tap part
 * This file is part of the command-line vesion of Audiotap
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <malloc.h>
#include <signal.h>

#include "audio2tap_core.h"
#include "audiotap.h"

void sig_int(int signum){
	audio2tap_interrupt();
}

void help(){
  printf("Usage: audio2tap -h|-V\n");
  printf("       audio2tap [-i] [-d min_duration] [-H sensitivity] [-f <freq>] [-0] [-c c64ntsc|vicpal|vicntsc|c16pal|c16ntsc] <output TAP file> [input WAV file]\n");
  printf("Options:\n");
  printf("\t-h: show this help message and exit successfully\n");
  printf("\t-V: show version and copyright info and exit successfully\n");
  printf("\t-i: use inverted waveforms (ignored if -c c16semi present)\n");
  printf("\t-d <min_duration>: ignore pulses if distance between min and max is less than or equal to <min_duration> samples (default:0)\n");
  printf("\t-H <sensitivity>: set sensitivity to <sensitivity> (range 0-100, default:12)\n");
  printf("\t-f: use input frequency <freq> Hz, default 44100 (only if input is sound card)\n");
  printf("\t-0: Generate a TAP file of version 0\n");
  printf("\t-c <c64|c16|c16semi|vic20>: Set clock frequency to match specified Commodore machine (default c64)\n");
  printf("\t-n: use NTSC timing\n");
  printf("\t-t <initial_threshold>: ignore initial pulses until their amplitude exceeds a range centred around <initial_threshold> (0-255, default:20)\n");
}

void version(){
  printf("audio2tap (part of Audiotap) version 1.4\n");
  printf("(C) by Fabrizio Gennari, 2003-2008\n");
  printf("This program is distributed under the GNU General Public License\n");
  printf("Read the file LICENSE.TXT for details\n");
  printf("This product includes software developed by the NetBSD\n");
  printf("Foundation, Inc. and its contributors\n");
}
   
int main(int argc, char** argv){
  struct audiotap_init_status status;
  unsigned int min_duration = 0;
  unsigned char sensitivity = 12;
  u_int32_t freq = 44100;
  int inverted = 0;
  unsigned char tap_version = 1;
  struct option cmdline[]={
    {"help"              ,0,NULL,'h'},
    {"version"           ,0,NULL,'V'},
    {"inverted-waveform" ,0,NULL,'i'},
    {"min-duration"      ,1,NULL,'d'},
    {"min-height"        ,1,NULL,'H'},
    {"freq"              ,1,NULL,'f'},
    {"tap-version-0"     ,0,NULL,'0'},
    {"clock"             ,1,NULL,'c'},
    {"ntsc"              ,0,NULL,'n'},
    {"initial-threshold" ,1,NULL,'t'},
    {NULL                ,0,NULL,0}
  };
  char *infile, *outfile;
  int option;
  int clock=0;
  uint8_t videotype = TAP_VIDEOTYPE_PAL;
  uint8_t initial_threshold = 20;

  status = audiotap_initialize();
  if (status.audiofile_init_status != LIBRARY_OK &&
      status.pablio_init_status != LIBRARY_OK){
    printf("Failed to initialize audiotap library: both audiofile and pablio failed to load");
    exit(1);
  }
  
  while( (option=getopt_long(argc,argv,"d:H:0ihVc:nw:t:",cmdline,NULL)) != -1){
    switch(option){
    case 'c':
      if(!strcmp(optarg,"c64"))
        clock=0;
      else if(!strcmp(optarg,"vic"))
        clock=1;
      else if(!strcmp(optarg,"c16"))
        clock=2;
      else if(!strcmp(optarg,"c16semi"))
        clock=3;
      else{
        printf("Wrong argument to option -c\n");
        exit(1);
      }
      break;
    case 'd':
      min_duration=atoi(optarg);
      break;
    case 't':
      initial_threshold=atoi(optarg);
      break;
    case 'H':
      sensitivity=atoi(optarg);
      if (sensitivity > 100){
        printf("Wrong argument to option -H, must be in range 0-100\n");
        exit(1);
      }
      break;
    case 'f':
      freq=atoi(optarg);
      break;
    case '0':
      tap_version=0;
      break;
    case 'i':
      inverted=1;
      break;
    case 'n':
      videotype=TAP_VIDEOTYPE_NTSC;
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

  if(argc == 0){
    printf("You must specify the output file name!\n");
    exit(1);
  }

  if(strlen(argv[0]) < 5 ||
     (strcmp(argv[0]+strlen(argv[0])-4,".tap") &&
      strcmp(argv[0]+strlen(argv[0])-4,".TAP"))){
    outfile=malloc(strlen(argv[0]+5));
    if (outfile == NULL){
      printf("Not enough memory!\n");
      exit(1);
    }
    sprintf(outfile, "%s.tap", argv[0]);
  }
  else outfile = argv[0];

  if (argc == 1){
    if (status.pablio_init_status != LIBRARY_OK){
      printf("Cannot read from sound card: pablio library missing or invalid\n");
      exit(1);
    }
    infile=NULL;
  }
  else{
    if (status.audiofile_init_status != LIBRARY_OK){
      printf("Cannot read from file: audiofile library missing or invalid\n");
      exit(1);
    }
    infile=argv[1];
  }      
  signal(SIGINT, sig_int);

  audio2tap(infile, outfile, freq, min_duration, sensitivity, inverted, initial_threshold, tap_version, clock, videotype);

  exit(0);
}

