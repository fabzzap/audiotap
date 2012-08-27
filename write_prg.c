#include "wav2prg_block_list.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <ctype.h>

static int isvocal(char c){
  return memchr("AEIOU", c, 5) ? 1 : 0;
}

static int ReduceName(const char *sC64Name, char *pDosName){
  int iStart;
  char sBuf[16 + 1];
  size_t iLen = strlen(sC64Name);
  int i;
  char *p = pDosName;
#if (defined MSDOS || defined _WINDOWS)
  int hFile;
#endif

  if (iLen > 16) {
    return 0;
  }

  if (strpbrk(sC64Name, "*?")) {
    strcpy(pDosName, "*");
    return 1;
  }

  memset(sBuf, 0, 16);
  strcpy(sBuf, sC64Name);

  for (i = 0; i <= 15; i++) {
    switch (sBuf[i]) {
    case ' ':
    case '-':
      sBuf[i] = '_';
      break;
    default:
      if (islower(sBuf[i])) {
        sBuf[i] -= 32;
        break;
      }

      if (isalnum(sBuf[i])) {
        break;
      }

      if (sBuf[i]) {
        sBuf[i] = 0;
        iLen--;
      }
    }
  }

  if (iLen <= 8) {
    goto Copy;
  }

  for (i = 15; i >= 0; i--) {
    if (sBuf[i] == '_') {
      sBuf[i] = 0;
      if (--iLen <= 8) {
        goto Copy;
      }
    }
  }

  for (iStart = 0; iStart < 15; iStart++) {
    if (sBuf[iStart] && !isvocal(sBuf[iStart])) {
      break;
    }
  }

  for (i = 15; i >= iStart; i--) {
    if (isvocal(sBuf[i])) {
      sBuf[i] = 0;
      if (--iLen <= 8) {
        goto Copy;
      }
    }
  }

  for (i = 15; i >= 0; i--) {
    if (isalpha(sBuf[i])) {
      sBuf[i] = 0;
      if (--iLen <= 8) {
        goto Copy;
      }
    }
  }

  for (i = 0; i <= 15; i++) {
    if (sBuf[i]) {
      sBuf[i] = 0;
      if (--iLen <= 8) {
        goto Copy;
      }
    }
  }
Copy:

  if (!iLen) {
    strcpy(pDosName, "_");
    return 1;
  }

  for (i = 0; i <= 15; i++) {
    if (sBuf[i]) {
      *p++ = sBuf[i];
    }
  }
  *p = 0;

  /* overcome limitations of FAT filesystem.
     A nicer, more portable way of finding names
     not accepted by FAT is needed */
#if (defined MSDOS || defined _WINDOWS)
  hFile = open(pDosName, O_RDONLY);
  if (hFile == -1) {
    return 1;
  }

  if (isatty(hFile)) {
    if (iLen < 8) {
      strcat(pDosName, "_");
    }
    else if (pDosName[7] != '_') {
      pDosName[7] = '_';
    }
    else {
      pDosName[7] = 'X';
    }
  }

  close(hFile);
#endif

  return 1;
}

static void strip_trailing_spaces(const char *name, char *name_nosp)
{
  int i, j, k = 0;

  for (i = 15; i >= 0; i--)
    if (name[i] != ' ')
      break;

  for (j = 0; j <= i; j++){
    if (name[j] >= 32
     && name[j] != '/'
#ifdef WIN32
     && name[j] != '<'
     && name[j] != '>'
     && name[j] != ':'
     && name[j] != '"'
     && name[j] != '\\'
     && name[j] != '|'
     && name[j] != '?'
     && name[j] != '*'
#endif
      )
      name_nosp[k++] = name[j];
    else if(name[j] >= -64 && name[j] <= -34)
      name_nosp[k++] = name[j] + 160;
    else if(name[j] == -1)
      name_nosp[k++] = 126;
  }
  name_nosp[k] = 0;
}

void write_prg(struct block_list_element *blocks, const char *dirname, enum wav2prg_bool use_p00){
  char *extension, *filename, *fullpathname;
  int fildes = 0, i;

  while(blocks){
    fullpathname = malloc(strlen(dirname) + 25);

    sprintf(fullpathname, "%s/", dirname);
    filename = fullpathname + strlen(fullpathname);

    if (use_p00) {
      char p00_header[]=
      {'C', '6', '4', 'F', 'i', 'l', 'e', 0
       , 0, 0, 0, 0, 0, 0, 0, 0
       , 0, 0, 0, 0, 0, 0, 0, 0
       , 0, 0};
      if (ReduceName(blocks->block.info.name, filename) == 1) {
        extension = filename + strlen(filename);
        for (i = 0; i < 100; i++) {
          sprintf(extension, ".p%02d", i);
          /* What a pity: fopen does not support the flag "x" on all platforms */
    #if (defined WIN32 || defined __CYGWIN__)
          fildes =
            open(fullpathname, O_WRONLY | O_CREAT | O_EXCL | O_BINARY,
                 S_IREAD | S_IWRITE);
    #else
          fildes =
            open(fullpathname, O_WRONLY | O_CREAT | O_EXCL,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    #endif
          if (fildes > 0 || errno != EEXIST)
            break;
        }
        if (fildes > 0){
          memcpy(p00_header + 8, blocks->block.info.name, 16);
          if (write(fildes, p00_header, sizeof(p00_header)) < sizeof(p00_header)){
            close(fildes);
            unlink(fullpathname);
            fildes=0;
          }
        }
      }
    }
    else{
      char name_nospaces[17];
      strip_trailing_spaces(blocks->block.info.name, name_nospaces);
      if (!strlen(name_nospaces))
        strcpy(filename, "default");
      else
        strcpy(filename, name_nospaces);
      extension = filename + strlen(filename);
      for (i = 0; i < 100; i++) {
        if (i == 0)
          strcpy(extension, ".prg");
        else
          sprintf(extension, "_%d.prg", i);
        fildes =
  #if (defined WIN32 || defined __CYGWIN__)
              open(fullpathname, O_WRONLY | O_CREAT | O_EXCL | O_BINARY,
                   S_IREAD | S_IWRITE);
  #else
              open(fullpathname, O_WRONLY | O_CREAT | O_EXCL,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  #endif
        if (fildes > 0 || errno != EEXIST)
          break;
      }
    }
    if (fildes > 0) {
      char start_addr[2] =
      {
         blocks->real_start       & 0xFF,
        (blocks->real_start >> 8) & 0xFF
      };
      write(fildes, start_addr, sizeof(start_addr));
      write(fildes, blocks->block.data, blocks->real_end - blocks->real_start);
      close(fildes);
    }
    free(fullpathname);
    blocks = blocks->next;
  }
}
