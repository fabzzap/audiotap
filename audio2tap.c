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

int main(int argc, char** argv){
  struct audiotap_init_status status;
  unsigned int min_duration = 2;
  unsigned char min_height = 10;
  u_int32_t freq = 44100;
  int inverted = 0;
  unsigned char tap_version = 1;
  struct option cmdline[]={
    {"min-duration"      ,1,NULL,'d'},
    {"min-height"        ,1,NULL,'h'},
    {"tap-version-0"     ,0,NULL,'0'},
    {"inverted-waveform" ,0,NULL,'i'},
    {"freq"              ,1,NULL,'f'},
    {NULL                ,0,NULL,0}
  };
  char *infile, *outfile;
  int option;

  status = audiotap_initialize();
  if (status.audiofile_init_status != LIBRARY_OK &&
      status.pablio_init_status != LIBRARY_OK){
    printf("Failed to initialize audiotap library: both audiofile and pablio failed to load");
    exit(1);
  }
  
  while( (option=getopt_long(argc,argv,"d:h:0i",cmdline,NULL)) != -1){
    switch(option){
    case 'd':
      min_duration=atoi(optarg);
      break;
    case 'h':
      min_height=atoi(optarg);
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
    default:
      printf("Unknown option\n");
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

  audio2tap(infile, outfile, freq, min_duration, min_height << 24, inverted, tap_version);

  exit(0);
}
