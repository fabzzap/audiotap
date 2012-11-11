/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audiotap_callback.h : prototypes for functions called by the core
 * processing to notify user about progress/info/warnings/errors...
 * These functions are implemented in different ways by different user
 * interfaces
 * 
 * This file is shared between the audio->tap part and the tap->audio part
 * This file is part of Audiotap core files
 */

void warning_message(const char *format,...);
void error_message(const char *format,...);

void update_input_filename(const char *input_filename);

void statusbar_initialize(int length);
void statusbar_update(int cursize);
void statusbar_exit(void);
