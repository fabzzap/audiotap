/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audiotap_callback.c : function called by the core processing to
 * notify the user about progress/info/errors...
 * 
 * The progress bar functions are based on code used by scp and sftp,
 * parts of the OpenSSH suite. Copyright notice for such code follows:
 * 
 * Copyright (c) 1997-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the NetBSD
 *  Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE 

 * This file is shared between the audio->tap part and the tap->audio part
 * This file is part of the command-line version of Audiotap
 */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>

void warning_message(const char *format,...){
  char string[80];
  va_list va;

  va_start(va, format);
  vsnprintf(string, 80, format, va);
  printf("Warning: %s\n", string);
  va_end(va);
}

void error_message(const char *format,...){
  char string[80];
  va_list va;

  va_start(va, format);
  vsnprintf(string, 80, format, va);
  printf("Error: %s\n", string);
  va_end(va);
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

int statusbar_length = 0;

static int
foregroundproc(void)
{
        static pid_t pgrp = -1;
        int ctty_pgrp;

        if (pgrp == -1)
                pgrp = getpgrp();

        return ((ctty_pgrp = tcgetpgrp(STDOUT_FILENO)) != -1 &&
                ctty_pgrp == pgrp);

}

static int
getttywidth(void)
{
#ifdef TIOCGWINSIZE
        struct winsize winsize;
        if (ioctl(fileno(stdout), TIOCGWINSZ, &winsize) != -1)
                return (winsize.ws_col ? winsize.ws_col : 80);
        else
#endif
                return (80);
}

void
statusbar_update(int cursize)
{
        int ratio, barlength, i;
        char buf[512];

        if (foregroundproc() == 0)
                return;

        if (statusbar_length != 0) {
                ratio = 100.0 * cursize / statusbar_length;
                ratio = MAX(ratio, 0);
                ratio = MIN(ratio, 100);
        } else
                ratio = 100;

        snprintf(buf, sizeof(buf), "\r%3d%% ", ratio);

        barlength = getttywidth() - 8;
        if (barlength > 0) {
                i = barlength * ratio / 100;
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                    "|%.*s%*s|", i,
                    "*******************************************************"
                    "*******************************************************"
                    "*******************************************************"
                    "*******************************************************"
                    "*******************************************************"
                    "*******************************************************"
                    "*******************************************************",
                    barlength - i, "");
        }
        i = 0;

        write(fileno(stdout), buf, strlen(buf));
}

void statusbar_initialize(int length){
  statusbar_length=length;
  statusbar_update(0);
}

void statusbar_exit(void){
  write(fileno(stdout),"\n",1);
}
