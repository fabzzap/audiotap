/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audiotap.c : main program for Windows version
 * Also implements the callback functions for the core processing part
 *
 * This file is shared between the audio->tap part and the tap->audio part
 * This file is part of the Windows GUI version of Audiotap
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_HTMLHELP
#include <htmlhelp.h>
#endif
#ifndef _MSC_VER
#include <libgen.h>
#endif
#include "resource.h"
#include "audiotap.h"
#include "audiotap_loop.h"
#include "tap2audio_core.h"
#include "audio2tap_core.h"

static struct audiotap_init_status audiotap_status;
static HINSTANCE instance;
static HWND status_window = NULL;

void warning_message(const char *format,...){
  char string[160];
  va_list va;

  va_start(va, format);
    if (_vsnprintf(string, 160, format, va) == -1)
    string[159]=0;
  MessageBoxA(status_window, string, "Audiotap warning", MB_ICONWARNING | MB_SETFOREGROUND);
  va_end(va);
}

void error_message(const char *format,...){
  char string[160];
  va_list va;

  va_start(va, format);
  if (_vsnprintf(string, 160, format, va) == -1)
    string[159]=0;
  MessageBoxA(status_window, string, "Audiotap error", MB_ICONERROR | MB_SETFOREGROUND);
  va_end(va);
}

static double statusbar_factor;

void
statusbar_update(int cursize)
{
  if (status_window == NULL)
    return;
  SendMessage(GetDlgItem(status_window, IDC_PROGRESSBAR),
    PBM_SETPOS,
    (WPARAM)((unsigned int)(cursize*statusbar_factor)),
    0);
  UpdateWindow(status_window);
}

void statusbar_initialize(int length)
{
  statusbar_factor = 65535.0/length;
  if (status_window == NULL)
    return;
  SendMessage(GetDlgItem(status_window, IDC_PROGRESSBAR),
    PBM_SETRANGE,
    0,
    MAKELPARAM(0,65535));
  SendMessage(GetDlgItem(status_window, IDC_PROGRESSBAR),
    PBM_SETPOS,
    0,
    0);
}

void statusbar_exit(void){
}

INT_PTR CALLBACK status_window_proc( HWND hwndDlg,
 // handle to dialog box

UINT uMsg,
 // message

WPARAM wParam,
 // first message parameter

LPARAM lParam
 // second message parameter

){
  switch (uMsg)
  {
  case WM_COMMAND:
    if (LOWORD(wParam) == IDC_STOP){
      audiotap_interrupt();
      return 1;
    }
    return 0;
  default:
    return 0;
  }

}

static void set_library_status_message(enum library_status status, HWND hwnd, int item){
  char *message;
  switch (status){
  case LIBRARY_UNINIT:
    message="Not initialized";
    break;
  case LIBRARY_MISSING:
    message="Not present or incorrectly installed";
    break;
  case LIBRARY_SYMBOLS_MISSING:
    message="Present, but invalid: maybe old version";
    break;
  case LIBRARY_INIT_FAILED:
    message="Present, but initialization failed";
    break;
  case LIBRARY_OK:
    message="OK";
    break;
  }
  SetWindowTextA(GetDlgItem(hwnd, item), message);
}

static INT_PTR CALLBACK about_proc(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){
  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      set_library_status_message(audiotap_status.portaudio_init_status, hwnd, IDC_PABLIO_STATUS);
      set_library_status_message(audiotap_status.audiofile_init_status, hwnd, IDC_AUDIOFILE_STATUS);
      set_library_status_message(audiotap_status.tapencoder_init_status, hwnd, IDC_TAPENCODER_STATUS);
      set_library_status_message(audiotap_status.tapdecoder_init_status, hwnd, IDC_TAPDECODER_STATUS);
      return 1;
    }
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK){
      EndDialog(hwnd,0);
      return 1;
    }
  default:
    return 0;
  }
}

struct audiotap_advanced {
  char **input_filenames;
  unsigned int num_input_filenames;
  char to_audio;
  char input_filename[1024];
  char output_filename[1024];
  uint32_t freq;
  uint8_t machine;
  uint8_t videotype;
  uint8_t tap_version;
  struct tapenc_params tapenc_params;
  struct tapdec_params tapdec_params;
};

UINT_PTR APIENTRY wav_opensave_hook_proc ( HWND hdlg,
 // handle to child dialog window

UINT uiMsg,
 // message identifier

WPARAM wParam,
 // message parameter

LPARAM lParam
 // message parameter

 ){
  if (uiMsg == WM_NOTIFY){
    OFNOTIFY *notify = (OFNOTIFY *)lParam;
    if (notify->hdr.code == CDN_TYPECHANGE){
      switch(notify->lpOFN->nFilterIndex){
      case 1:
        CommDlg_OpenSave_SetDefExt(GetParent(hdlg), "wav");
        break;
      case 2:
        CommDlg_OpenSave_SetDefExt(GetParent(hdlg), NULL);
      default:
        ;
      }

    }
  }
  return 0;
}

UINT_PTR APIENTRY tap_open_hook_proc ( HWND hdlg,
 // handle to child dialog window

UINT uiMsg,
 // message identifier

WPARAM wParam,
 // message parameter

LPARAM lParam
 // message parameter

 ){
  if (uiMsg == WM_NOTIFY){
    OFNOTIFY *notify = (OFNOTIFY *)lParam;
    if (notify->hdr.code == CDN_TYPECHANGE){
      switch(notify->lpOFN->nFilterIndex){
      case 1:
        CommDlg_OpenSave_SetDefExt(GetParent(hdlg), "tap");
        break;
      case 2:
        CommDlg_OpenSave_SetDefExt(GetParent(hdlg), NULL);
      default:
        ;
      }

    }
  }
  return 0;
}

UINT_PTR APIENTRY tap_save_hook_proc ( HWND hdlg,
 // handle to child dialog window

UINT uiMsg,
 // message identifier

WPARAM wParam,
 // message parameter

LPARAM lParam
 // message parameter

 ){
  if (uiMsg == WM_INITDIALOG){
    struct audiotap_advanced *adv = (struct audiotap_advanced *)((LPOPENFILENAME)lParam)->lCustData;
    SendMessageA(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_ADDSTRING, 0, (LPARAM)"Version 0");
    SendMessageA(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_ADDSTRING, 0, (LPARAM)"Version 1");
    SendMessageA(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_ADDSTRING, 0, (LPARAM)"Version 2");
    SendMessageA(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_SETCURSEL, (WPARAM)adv->tap_version, 0);
  }
  if (uiMsg == WM_NOTIFY){
    LPOFNOTIFY notify = (LPOFNOTIFY)lParam;
    if (notify->hdr.code == CDN_TYPECHANGE){
      switch(notify->lpOFN->nFilterIndex){
      case 1:
        CommDlg_OpenSave_SetDefExt(GetParent(hdlg), "tap");
        break;
      case 2:
        CommDlg_OpenSave_SetDefExt(GetParent(hdlg), NULL);
      default:
        ;
      }
    }
    if (notify->hdr.code == CDN_FILEOK){
      HWND main_window = GetParent(GetParent(hdlg));
      struct audiotap_advanced *adv = (struct audiotap_advanced *)notify->lpOFN->lCustData;
      if (adv != NULL){
        LRESULT tap_result = SendMessage(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_GETCURSEL, 0, 0);
        adv->tap_version = (tap_result >= 0 && tap_result <= 2) ? (uint8_t)tap_result : 1;
      }
    }
  }
  return 0;
}

DWORD WINAPI audio2tap_thread(LPVOID params){
  audio2tap(((struct audiotap_advanced*)params)->input_filenames,
    ((struct audiotap_advanced*)params)->num_input_filenames,
    ((struct audiotap_advanced*)params)->output_filename,
    ((struct audiotap_advanced*)params)->freq,
    &((struct audiotap_advanced*)params)->tapenc_params,
    ((struct audiotap_advanced*)params)->tap_version,
    ((struct audiotap_advanced*)params)->machine,
    ((struct audiotap_advanced*)params)->videotype);
  return 0;
}

static void get_basename(const char *filename, char *output_name, int outlen)
{
#ifdef _MSC_VER
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

  _splitpath(filename, NULL, NULL, fname, ext);
  strncpy(output_name, fname, outlen);
  strncat(output_name, ext, outlen);
#else
  char *writable_filename = strdup(filename);
  /* why, oh why cannot the argument of basename () be const? */
  strncpy(output_name, basename(writable_filename), outlen);
  free(writable_filename);
#endif
}

void update_input_filename(const char *input_filename)
{
  char msg_string[128] =  "Origin: ";
  get_basename(input_filename, msg_string + strlen(msg_string), sizeof(msg_string) - strlen(msg_string));
  SetWindowTextA(GetDlgItem(status_window, IDC_ORIGIN), msg_string);
}

void real_save_to_tap(HWND parent, struct audiotap_advanced *adv)
{
  MSG msg;
  DWORD thread_id;
  HANDLE thread;
  char msg_string[128];
  HWND control;
  OPENFILENAMEA file;
  char *output_filename_base;
  unsigned int i;

  adv->output_filename[0]=0;
  memset(&file,0,sizeof(file));
  file.lStructSize = sizeof(file);
  file.hwndOwner = parent;
  file.nMaxFile = sizeof(adv->output_filename);

  file.lpstrFilter ="TAP file (*.tap)\0*.tap\0All files\0*.*\0\0";
  file.lpstrTitle = "Choose the TAP file to be created";
  file.Flags = OFN_EXPLORER | OFN_HIDEREADONLY |
    OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK;
  file.Flags |= OFN_ENABLETEMPLATE;
  file.lpTemplateName = MAKEINTRESOURCEA(IDD_CHOOSE_TAP_VERSION);
  file.lpstrFile = adv->output_filename;
  file.hInstance = instance;
  file.lpfnHook = tap_save_hook_proc;
  file.lpstrDefExt = "tap";
  file.lCustData = (LPARAM)adv;
  if (GetSaveFileNameA(&file) == TRUE){
    output_filename_base = adv->output_filename + file.nFileOffset;

    status_window=CreateDialogA(instance,MAKEINTRESOURCEA(IDD_STATUS),parent,status_window_proc);
    EnableWindow(parent, FALSE);
    ShowWindow(status_window, SW_SHOWNORMAL);
    UpdateWindow(status_window);
    if(adv->num_input_filenames == 0)
      SetWindowTextA(GetDlgItem(status_window, IDC_ORIGIN), "Origin: sound card");
    _snprintf(msg_string, 128, "Destination: %s", output_filename_base);
    control=GetDlgItem(status_window, IDC_DESTINATION);
    SetWindowTextA(control, msg_string);
    strncpy(msg_string, IsDlgButtonChecked(parent, IDC_FROM_WAV) ? "Progress indication" : "Volume level", 128);
    control=GetDlgItem(status_window, IDC_WHAT_PROGRESSBAR_MEANS);
    SetWindowTextA(control, msg_string);

    adv->tapenc_params.inverted = (IsDlgButtonChecked(parent, IDC_TO_TAP_INVERTED) == BST_CHECKED);
      thread=CreateThread(NULL, 0, audio2tap_thread, adv, 0, &thread_id);

    while(1){
      DWORD retval;
      retval = MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT);
      if (retval == WAIT_OBJECT_0)
        break;
      while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
      {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
      }
    }

    EnableWindow(parent, TRUE);
    DestroyWindow(status_window);
    status_window = NULL;
  }
  for (i = 0; i < adv->num_input_filenames; i++)
  {
    free(adv->input_filenames[i]);
  }
  free(adv->input_filenames);
}

void save_to_tap(HWND hwnd){
  struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(hwnd, GWL_USERDATA);

  adv->input_filenames = NULL;
  adv->num_input_filenames = 0;

  if (IsDlgButtonChecked(hwnd, IDC_FROM_WAV)){
    OPENFILENAMEA file;
    char input_filename[1024];
    int dir_len;

    input_filename[0]=0;
    memset(&file,0,sizeof(file));
    file.lStructSize = sizeof(file);
    file.hwndOwner = hwnd;
    file.nMaxFile = 1024;  
    file.lpstrFilter ="WAV files (*.wav)\0*.wav\0All files\0*.*\0\0";
    file.lpstrTitle = "Choose the audio file (WAV or similar) to convert to TAP";
    file.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_ALLOWMULTISELECT;
    file.lpstrFile = input_filename;
    file.lpfnHook = wav_opensave_hook_proc;
    file.lpstrDefExt = "wav";
    file.nFilterIndex = 1;
    if (GetOpenFileNameA(&file) == FALSE)
      return;
    if (file.nFileOffset < 1)
      return;
    dir_len = strlen(input_filename) + 1;

    if (input_filename[file.nFileOffset - 1] != 0){
      adv->input_filenames = (char**)malloc(sizeof(*adv->input_filenames));
      adv->input_filenames[0] = (char*)malloc(dir_len);
      strcpy(adv->input_filenames[0], input_filename);
      adv->num_input_filenames = 1;
    }
    else
    {
      char *filename;
      int filename_len;

      for (filename = input_filename + dir_len; (filename_len = strlen(filename) + 1) != 1; filename += filename_len, adv->num_input_filenames++){
        adv->input_filenames = (char**)realloc(adv->input_filenames, sizeof(*adv->input_filenames) * (adv->num_input_filenames + 1));
        adv->input_filenames[adv->num_input_filenames] = (char*)malloc(dir_len + filename_len);
        strcpy(adv->input_filenames[adv->num_input_filenames], input_filename);
        strcat(adv->input_filenames[adv->num_input_filenames], "\\");
        strcat(adv->input_filenames[adv->num_input_filenames], filename);
      }
    }
  }
  real_save_to_tap(hwnd, adv);
}

struct play_pause_icon {
  BOOL pause;
  HBITMAP play_icon;
  HBITMAP pause_icon;
};

INT_PTR CALLBACK tap2audio_status_window_proc( HWND hwndDlg,
 // handle to dialog box

UINT uMsg,
 // message

WPARAM wParam,
 // first message parameter

LPARAM lParam
 // second message parameter

){
  switch (uMsg)
  {
  case WM_COMMAND:
    if (LOWORD(wParam) == IDC_STOP) {
      audiotap_interrupt();
      audiotap_resume();
      return 1;
    }
    if (LOWORD(wParam) == IDC_PLAYPAUSE) {
      struct play_pause_icon *icon = (struct play_pause_icon *)GetWindowLongPtr(hwndDlg, GWL_USERDATA);
      if (icon->pause){
        audiotap_resume();
        SendDlgItemMessage(hwndDlg, IDC_PLAYPAUSE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)icon->pause_icon);
      }
      else {
        audiotap_pause();
        SendDlgItemMessage(hwndDlg, IDC_PLAYPAUSE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)icon->play_icon);
      }
      icon->pause = !icon->pause;
      return 1;
    }
  default:
    return 0;
  }
}

DWORD WINAPI tap2audio_thread(LPVOID params){
  tap2audio(((struct audiotap_advanced*)params)->input_filename,
    ((struct audiotap_advanced*)params)->output_filename,
    ((struct audiotap_advanced*)params)->to_audio,
    &((struct audiotap_advanced*)params)->tapdec_params,
    ((struct audiotap_advanced*)params)->freq);
  return 0;
}

void read_from_tap(HWND hwnd){
  OPENFILENAMEA file;
  char* input_filename_base, *output_filename_base;
  MSG msg;
  DWORD thread_id;
  HANDLE thread;
  char msg_string[128];
  struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(hwnd, GWLP_USERDATA);
  struct play_pause_icon icon = { FALSE };
  HBITMAP stop_icon;
  BOOL is_to_sound;

  adv->input_filename[0]=0;
  adv->output_filename[0]=0;
  adv->to_audio=1;
  memset(&file,0,sizeof(file));
  file.lStructSize = sizeof(file);
  file.hwndOwner = hwnd;
  file.nMaxFile = sizeof(adv->input_filename);

  file.lpstrFilter ="TAP files (*.tap)\0*.tap\0DMP files (*.dmp)\0*.dmp\0All files\0*.*\0\0";
  file.lpstrDefExt = "tap";
  file.nFilterIndex = 1;
  file.lpstrTitle = "Choose the TAP file to be converted to WAV";
  file.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
  file.lpstrFile = adv->input_filename;
  file.lpfnHook = tap_open_hook_proc;
  if (GetOpenFileNameA(&file) == FALSE)
    return;
  input_filename_base = adv->input_filename + file.nFileOffset;

  if (IsDlgButtonChecked(hwnd, IDC_TO_WAV)){
    file.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLEHOOK |
    OFN_OVERWRITEPROMPT;
    file.lpstrFile = adv->output_filename;
    file.lpstrFilter ="WAV files (*.wav)\0*.wav\0All files\0*.*\0\0";
    file.lpstrTitle = "Choose the name of the WAV file to be created";
    file.lpstrDefExt = "wav";
    file.nFilterIndex = 1;
    file.lpfnHook = wav_opensave_hook_proc;
    if (GetSaveFileNameA(&file) == FALSE)
      return;
    output_filename_base = adv->output_filename + file.nFileOffset;
    adv->to_audio=0;
  }

  status_window=CreateDialog(instance,MAKEINTRESOURCE(IDD_STATUS),hwnd,tap2audio_status_window_proc);
  SetWindowLongPtr(status_window, GWLP_USERDATA, (LONG_PTR)&icon);
  is_to_sound = IsDlgButtonChecked(hwnd, IDC_TO_SOUND);
  if (is_to_sound) {
    HWND stop_button = GetDlgItem(status_window, IDC_STOP);
    RECT window_rect, play_pause_button_rect;
    stop_icon = LoadBitmap(instance, MAKEINTRESOURCE(IDB_STOP));
    icon.play_icon = LoadBitmap(instance, MAKEINTRESOURCE(IDB_PLAY));
    icon.pause_icon = LoadBitmap(instance, MAKEINTRESOURCE(IDB_PAUSE));
    /* make sure the pause button is visible and shows the pause icon*/
    SendDlgItemMessage(status_window, IDC_PLAYPAUSE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)icon.pause_icon);
    ShowWindow(GetDlgItem(status_window, IDC_PLAYPAUSE), SW_SHOW);
    /* get width of window */
    GetClientRect(status_window, &window_rect);
    /* get absolute position of play-pause button */
    GetWindowRect(GetDlgItem(status_window, IDC_PLAYPAUSE), &play_pause_button_rect);
    /* get position of play-pause button within window */
    MapWindowPoints(NULL, status_window, (LPPOINT)&play_pause_button_rect, 2);
    /* make stop button as big as play-pause button, top-aligned and symmetrically placed */
    MoveWindow(stop_button,
      window_rect.right - play_pause_button_rect.right,
      play_pause_button_rect.top,
      play_pause_button_rect.right - play_pause_button_rect.left,
      play_pause_button_rect.bottom - play_pause_button_rect.top,
      TRUE);
    /* make sure the stop button shows the stop icon instead of the word Cancel*/
    SetWindowLongPtr(stop_button, GWL_STYLE, GetWindowLongPtr(stop_button, GWL_STYLE) | BS_BITMAP);
    SendDlgItemMessage(status_window, IDC_STOP, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)stop_icon);
    LONG_PTR stile = GetWindowLongPtr(GetDlgItem(status_window, IDC_PLAYPAUSE), GWL_STYLE);
    stile |= WS_TABSTOP;
    SetWindowLongPtr(GetDlgItem(status_window, IDC_PLAYPAUSE), GWL_STYLE, stile);
  }
  EnableWindow(hwnd, FALSE);
  ShowWindow(status_window, SW_SHOWNORMAL);
  UpdateWindow(status_window);
  _snprintf(msg_string, 128, "Origin: %s", input_filename_base);
  SetWindowTextA(GetDlgItem(status_window, IDC_ORIGIN), msg_string);
  _snprintf(msg_string, 128, "Destination: %s", (IsDlgButtonChecked(hwnd, IDC_TO_WAV) ? output_filename_base : "sound card"));
  SetWindowTextA(GetDlgItem(status_window, IDC_DESTINATION), msg_string);
  strncpy(msg_string, "Progress indication", 128);
  SetWindowTextA(GetDlgItem(status_window, IDC_WHAT_PROGRESSBAR_MEANS), msg_string);

  adv->tapdec_params.inverted = (IsDlgButtonChecked(hwnd, IDC_FROM_TAP_INVERTED) == BST_CHECKED);

  thread=CreateThread(NULL, 0, tap2audio_thread, adv, 0, &thread_id);

  while(MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0){
    while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
    {
      if (is_to_sound && msg.message == WM_KEYDOWN) {
        if (msg.wParam == VK_MEDIA_PLAY_PAUSE)
          PostMessage (status_window, WM_COMMAND, IDC_PLAYPAUSE, 0);
        else if (msg.wParam == VK_MEDIA_STOP)
          PostMessage (status_window, WM_COMMAND, IDC_STOP, 0);
      }
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
  }

  EnableWindow(hwnd, TRUE);
  DestroyWindow(status_window);
  status_window = NULL;
  if (IsDlgButtonChecked(hwnd, IDC_TO_SOUND)){
    DeleteObject(icon.play_icon);
    DeleteObject(icon.pause_icon);
    DeleteObject(stop_icon);
  }
}

INT_PTR CALLBACK to_tap_advanced_proc(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){

  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      struct audiotap_advanced *adv = (struct audiotap_advanced *)lParam;
      SetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_FREQ        ,adv->freq                                     ,FALSE);
      SetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_HEIGHT  ,adv->tapenc_params.sensitivity      ,FALSE);
      SetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_DURATION,adv->tapenc_params.min_duration     ,FALSE);
      SetDlgItemInt(hwnd,IDC_INITIAL_THRESHOLD           ,adv->tapenc_params.initial_threshold,FALSE);
      SendMessageA(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"C64");
      SendMessageA(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"VIC20");
      SendMessageA(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"C16");
      if (adv->machine == TAP_MACHINE_C64)
        SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_SETCURSEL,0,0);
      else if (adv->machine == TAP_MACHINE_VIC)
        SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_SETCURSEL,1,0);
      else
        SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_SETCURSEL,2,0);
      if(adv->videotype == TAP_VIDEOTYPE_PAL)
        CheckRadioButton(hwnd,IDC_VIDEOTYPE_PAL,IDC_VIDEOTYPE_NTSC,IDC_VIDEOTYPE_PAL);
      else
        CheckRadioButton(hwnd,IDC_VIDEOTYPE_PAL,IDC_VIDEOTYPE_NTSC,IDC_VIDEOTYPE_NTSC);
      return 1;
    }
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK){
      struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
      BOOL success;
      LRESULT clock_result;
      adv->tapenc_params.sensitivity = GetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_HEIGHT,&success,FALSE);
      if (!success || adv->tapenc_params.sensitivity > 100)
        adv->tapenc_params.sensitivity = 12;
      adv->tapenc_params.min_duration = GetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_DURATION  ,&success,FALSE);
      if (!success)
        adv->tapenc_params.min_duration = 0;
      adv->freq = GetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_FREQ          ,&success,FALSE);
      if (!success)
        adv->freq = 44100;
      adv->tapenc_params.initial_threshold = GetDlgItemInt(hwnd,IDC_INITIAL_THRESHOLD,&success,FALSE);
      if (!success)
        adv->tapenc_params.min_duration = 20;
      clock_result = SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_GETCURSEL,0,0);
      switch (clock_result){
      case 0:
      default:
        adv->machine = TAP_MACHINE_C64;
        adv->tap_version = 1;
        break;
      case 1:
        adv->machine = TAP_MACHINE_VIC;
        adv->tap_version = 1;
        break;
      case 2:
        adv->machine = TAP_MACHINE_C16;
        adv->tap_version = 2;
        break;
      }
      if (IsDlgButtonChecked(hwnd, IDC_VIDEOTYPE_PAL) == BST_CHECKED)
        adv->videotype = TAP_VIDEOTYPE_PAL;
      else
        adv->videotype = TAP_VIDEOTYPE_NTSC;
      EndDialog(hwnd,0);
    }

    if (LOWORD(wParam) == IDCANCEL){
      EndDialog(hwnd,0);
    }
    return 1;
  default:
    return 0;
  }
}

INT_PTR CALLBACK from_tap_advanced_proc(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){

  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      struct audiotap_advanced *adv = (struct audiotap_advanced *)lParam;
      SetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_FREQ  ,adv->freq,FALSE);
      SetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_VOLUME,adv->tapdec_params.volume ,FALSE);
      SendMessageA(GetDlgItem(hwnd, IDC_WAVEFORM), CB_ADDSTRING, 0, (LPARAM)"Triangle");
      SendMessageA(GetDlgItem(hwnd, IDC_WAVEFORM), CB_ADDSTRING, 0, (LPARAM)"Square");
      SendMessageA(GetDlgItem(hwnd, IDC_WAVEFORM), CB_ADDSTRING, 0, (LPARAM)"Sine");
      /* The following needs that CB_ADDSTRING are called in the same order as in enum tapdec_waveform */
      SendMessage(GetDlgItem(hwnd, IDC_WAVEFORM), CB_SETCURSEL, (WPARAM)adv->tapdec_params.waveform, 0);
      return 1;
    }
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK){
      struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
      BOOL success;
      LRESULT waveform_result;
      adv->tapdec_params.volume = GetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_VOLUME,&success,FALSE);
      if (!success)
        adv->tapdec_params.volume = 254;
      adv->freq                 = GetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_FREQ  ,&success,FALSE);
      if (!success)
        adv->freq = 44100;
      waveform_result = SendMessage(GetDlgItem(hwnd,IDC_WAVEFORM),CB_GETCURSEL,0,0);
      switch(waveform_result){
      case 0:
        adv->tapdec_params.waveform = AUDIOTAP_WAVE_TRIANGLE;
        break;
      case 1:
      default:
        adv->tapdec_params.waveform = AUDIOTAP_WAVE_SQUARE;
        break;
      case 2:
        adv->tapdec_params.waveform = AUDIOTAP_WAVE_SINE;
        break;
      }
      EndDialog(hwnd,0);
    }
    if (LOWORD(wParam) == IDCANCEL){
      EndDialog(hwnd,0);
    }
  default:
    return 0;
  }
}

INT_PTR CALLBACK dialog_control(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){
  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      RECT desktop_rect, main_window_rect;
      BOOL retval1, retval2;
      retval1=GetWindowRect(GetDesktopWindow(), &desktop_rect    );
      retval2=GetWindowRect(hwnd,               &main_window_rect);
      if (retval1 && retval2){
        SetWindowPos(hwnd, 0,
          (desktop_rect.right -main_window_rect.right )/2,
          (desktop_rect.bottom-main_window_rect.bottom)/2,
          0,0,
          SWP_NOSIZE | SWP_NOZORDER);
      }
      SendMessage(hwnd, WM_SETICON, 1, (LPARAM)LoadIconA(instance, (LPCSTR)IDI_ICON));
    }
    SetWindowLong(hwnd, GWL_USERDATA, lParam);

    if (audiotap_status.portaudio_init_status == LIBRARY_OK){
      CheckRadioButton(hwnd,IDC_TO_SOUND,IDC_TO_WAV,IDC_TO_SOUND);
      CheckRadioButton(hwnd,IDC_FROM_SOUND,IDC_FROM_WAV,IDC_FROM_SOUND);
    }
    else{
      CheckRadioButton(hwnd,IDC_TO_SOUND,IDC_TO_WAV,IDC_TO_WAV);
      CheckRadioButton(hwnd,IDC_FROM_SOUND,IDC_FROM_WAV,IDC_FROM_WAV);
    }
    if (audiotap_status.tapdecoder_init_status == LIBRARY_OK)
      EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP), TRUE);
    if (audiotap_status.tapencoder_init_status == LIBRARY_OK)
      EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP), TRUE);
    if (audiotap_status.tapdecoder_init_status == LIBRARY_OK){
      CheckRadioButton(hwnd,IDC_FROM_TAP,IDC_TO_TAP,IDC_FROM_TAP);
      EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_INVERTED), TRUE);
      EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_ADVANCED), TRUE);
      if (audiotap_status.portaudio_init_status == LIBRARY_OK)
        EnableWindow(GetDlgItem(hwnd,IDC_TO_SOUND), TRUE);
      if (audiotap_status.audiofile_init_status == LIBRARY_OK)
        EnableWindow(GetDlgItem(hwnd,IDC_TO_WAV), TRUE);
    }
    else{
      CheckRadioButton(hwnd,IDC_FROM_TAP,IDC_TO_TAP,IDC_TO_TAP);
      EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_INVERTED), TRUE);
      EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_ADVANCED), TRUE);
      if (audiotap_status.portaudio_init_status == LIBRARY_OK)
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_SOUND), TRUE);
      if (audiotap_status.audiofile_init_status == LIBRARY_OK)
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_WAV), TRUE);
    }

    return TRUE;
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDC_FROM_TAP:
      {
        EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_ADVANCED), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_INVERTED), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_WAV), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_SOUND), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_ADVANCED), TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_INVERTED), TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_WAV), audiotap_status.audiofile_init_status == LIBRARY_OK);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_SOUND), audiotap_status.portaudio_init_status == LIBRARY_OK);
        return TRUE;
      }
    case IDC_TO_TAP:
      {
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_ADVANCED), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_INVERTED), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_WAV), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_SOUND), FALSE);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_ADVANCED), TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_INVERTED), TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_WAV), audiotap_status.audiofile_init_status == LIBRARY_OK);
        EnableWindow(GetDlgItem(hwnd,IDC_FROM_SOUND), audiotap_status.portaudio_init_status == LIBRARY_OK);
        return TRUE;
      }
    case IDOK:
      {
        if (IsDlgButtonChecked(hwnd, IDC_TO_TAP))
          save_to_tap(hwnd);
        else
          read_from_tap(hwnd);
        return TRUE;
      }
    case IDC_TO_TAP_ADVANCED:
      {
        struct audiotap_advanced adv;
        memcpy(&adv,(void*)GetWindowLong(hwnd,GWL_USERDATA),sizeof(adv));
        DialogBoxParam(instance, MAKEINTRESOURCE(IDD_TO_TAP_ADVANCED), hwnd, to_tap_advanced_proc, (LPARAM)&adv);
        return TRUE;
      }
    case IDC_FROM_TAP_ADVANCED:
      {
        struct audiotap_advanced adv;
        memcpy(&adv,(void*)GetWindowLong(hwnd,GWL_USERDATA),sizeof(adv));
        DialogBoxParam(instance, MAKEINTRESOURCE(IDD_FROM_TAP_ADVANCED), hwnd, from_tap_advanced_proc, (LPARAM)&adv);
        return TRUE;
      }
    case IDC_ABOUT:
      {
        DialogBox(instance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, about_proc);
        return TRUE;
      }
    default:
      return FALSE;
    }
  case WM_DROPFILES:
    {
      UINT i;
      struct audiotap_advanced adv;

      memcpy(&adv,(void*)GetWindowLong(hwnd,GWL_USERDATA),sizeof(adv));
      adv.num_input_filenames = DragQueryFileA((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
      adv.input_filenames = (char**)malloc(sizeof(*adv.input_filenames) * adv.num_input_filenames);
      for (i = 0; i < adv.num_input_filenames; i++)
      {
        UINT filenamesize = DragQueryFile((HDROP)wParam, i, NULL, 0);
        filenamesize++; /* for the null termination */
        adv.input_filenames[i] = (char*)malloc(filenamesize);
        DragQueryFileA((HDROP)wParam, i, adv.input_filenames[i], filenamesize);
      }
      DragFinish((HDROP)wParam);
      if (adv.num_input_filenames > 0)
      {
        real_save_to_tap(hwnd, &adv);
      }
    }
    return TRUE;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    return TRUE;
  case WM_DESTROY:
    /* The window is being destroyed, close the application
     * (the child button gets destroyed automatically). */
    PostQuitMessage (0);
    return TRUE;
#ifdef HAVE_HTMLHELP
  case WM_HELP:
    HtmlHelpA(hwnd,"docs\\audiotap.chm",HH_DISPLAY_TOC,0);
    return TRUE;
#endif
  default:
    return FALSE;
  }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
      LPSTR lpCmdLine, int nCmdShow ){
  struct audiotap_advanced adv = {NULL, 0, 0, "", "", 44100,
    TAP_MACHINE_C64,TAP_VIDEOTYPE_PAL,1,
    {0,12,20,0},
    {254,0,AUDIOTAP_WAVE_SQUARE}
  };

  instance = hInstance;
  audiotap_status = audiotap_initialize2();
  if (audiotap_status.audiofile_init_status != LIBRARY_OK &&
    audiotap_status.portaudio_init_status != LIBRARY_OK){
    MessageBoxA(0,"Both audiofile.dll and portaudio.dll are missing or improperly installed",
      "Cannot start Audiotap",MB_ICONERROR);
    return 0;
  }

  if (audiotap_status.tapdecoder_init_status != LIBRARY_OK &&
    audiotap_status.tapencoder_init_status != LIBRARY_OK){
      MessageBoxA(0,"Both tapencoder.dll and tapdecoder.dll are missing or improperly installed",
        "Cannot start Audiotap",MB_ICONERROR);
      return 0;
  }

  DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_MAINWINDOW),NULL,dialog_control,(LPARAM)&adv);
  return 0;
}
