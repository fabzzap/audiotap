/* Audiotap: a program for playing TAP files or converting them to WAV files,
 * and to convert C64 tapes to TAP files
 *
 * Copyright (c) Fabrizio Gennari, 2003
 *
 * The program is distributed under the GNU General Public License.
 * See file LICENSE.TXT for details.
 *
 * audiotap_main.c : main program for Windows version
 * Also implements the callback functions for the core processing part
 *
 * This file is shared between the audio->tap part and the tap->audio part
 * This file is part of the Windows GUI version of Audiotap
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include "resource.h"
#include "audiotap.h"
#include "tap2audio_core.h"
#include "audio2tap_core.h"

struct audiotap_init_status audiotap_status;
HINSTANCE instance;
HWND status_window = NULL;

void warning_message(const char *format,...){
  char string[160];
  va_list va;

  va_start(va, format);
    if (_vsnprintf(string, 160, format, va) == -1)
	  string[159]=0;
  MessageBox(0, string, "Audiotap warning", MB_ICONWARNING | MB_SETFOREGROUND);
  va_end(va);
}

void error_message(const char *format,...){
  char string[160];
  va_list va;

  va_start(va, format);
  if (_vsnprintf(string, 160, format, va) == -1)
	  string[159]=0;
  MessageBox(0, string, "Audiotap error", MB_ICONERROR | MB_SETFOREGROUND);
  va_end(va);
}

double statusbar_factor;

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

BOOL CALLBACK status_window_proc( HWND hwndDlg,
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
			audio2tap_interrupt();
			return 1;
		}
		return 0;
	default:
		return 0;
	}

};

BOOL CALLBACK about_proc(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			char *message;
			switch (audiotap_status.pablio_init_status){
			case LIBRARY_UNINIT:
				message="Not initialized";
				break;
			case LIBRARY_MISSING:
				message="Not present or incorrectly installed";
				break;
			case LIBRARY_SYMBOLS_MISSING:
				message="Present, but invalid: maybe old version";
				break;
			case LIBRARY_OK:
				message="OK";
				break;
			}
			SetWindowText(GetDlgItem(hwnd,IDC_PABLIO_STATUS),message);
			switch (audiotap_status.audiofile_init_status){
			case LIBRARY_UNINIT:
				message="Not initialized";
				break;
			case LIBRARY_MISSING:
				message="Not present or incorrectly installed";
				break;
			case LIBRARY_SYMBOLS_MISSING:
				message="Present, but invalid: maybe old version";
				break;
			case LIBRARY_OK:
				message="OK";
				break;
			}
			SetWindowText(GetDlgItem(hwnd,IDC_AUDIOFILE_STATUS),message);
			return 1;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK){
			EndDialog(hwnd,0);
		}
	default:
		return 0;
	}
}

struct audiotap_advanced {
	u_int32_t infreq;
	u_int32_t min_duration;
	u_int8_t  min_height;
	long      tap_version;
	u_int32_t outfreq;
	u_int8_t  volume;
	int       clock;
};

UINT APIENTRY wav_opensave_hook_proc ( HWND hdlg,
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
};

UINT APIENTRY tap_open_hook_proc ( HWND hdlg,
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
};

UINT APIENTRY tap_save_hook_proc ( HWND hdlg,
 // handle to child dialog window

UINT uiMsg,
 // message identifier

WPARAM wParam,
 // message parameter

LPARAM lParam
 // message parameter

 ){
	if (uiMsg == WM_INITDIALOG){
		SendMessage(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_ADDSTRING, 0, (LPARAM)"Version 0");
		SendMessage(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_ADDSTRING, 0, (LPARAM)"Version 1");
		SendMessage(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_SETCURSEL, (WPARAM)1, 0);
	}
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
		if (notify->hdr.code == CDN_FILEOK){
			HWND main_window = GetParent(GetParent(hdlg));
			struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(main_window, GWL_USERDATA);
			if (adv != NULL)
				adv->tap_version = SendMessage(GetDlgItem(hdlg, IDC_CHOOSE_TAP_VERSION), CB_GETCURSEL, 0, 0);
		}
	}
	return 0;
};

struct audio2tap_parameters {
	char *input_filename;
	char *output_filename;
	u_int32_t freq;
	u_int32_t min_duration;
	u_int32_t min_height;
	int inverted;
	unsigned char tap_version;
	int clock;
};

DWORD WINAPI audio2tap_thread(LPVOID params){
	audio2tap(((struct audio2tap_parameters*)params)->input_filename,
		((struct audio2tap_parameters*)params)->output_filename,
		((struct audio2tap_parameters*)params)->freq,
		((struct audio2tap_parameters*)params)->min_duration,
		((struct audio2tap_parameters*)params)->min_height,
		((struct audio2tap_parameters*)params)->inverted,
		((struct audio2tap_parameters*)params)->tap_version,
		((struct audio2tap_parameters*)params)->clock);
	return 0;
}

void save_to_tap(HWND hwnd){
	OPENFILENAME file;
	char input_filename[1024];
	char output_filename[1024];
	MSG msg;
	struct audio2tap_parameters params;
	DWORD thread_id;
	HANDLE thread;
	char msg_string[128];
	HWND control;
	struct audiotap_advanced *adv;

	input_filename[0]=0;
	output_filename[0]=0;
	memset(&file,0,sizeof(file));
	file.lStructSize = sizeof(file);
	file.hwndOwner = hwnd;
	file.nMaxFile = 1024;

	if (IsDlgButtonChecked(hwnd, IDC_FROM_WAV)){
		file.lpstrFilter ="WAV files (*.wav)\0*.wav\0All files\0*.*\0\0";
		file.lpstrTitle = "Choose the audio file (WAV or similar) to convert to TAP";
		file.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
		file.lpstrFile = input_filename;
		file.lpfnHook = wav_opensave_hook_proc;
		file.lpstrDefExt = "wav";
		file.nFilterIndex = 1;
		if (GetOpenFileName(&file) == FALSE)
			return;
	}

	file.lpstrFilter ="TAP file (*.tap)\0*.tap\0All files\0*.*\0\0";
	file.lpstrTitle = "Choose the TAP file to be created";
	file.Flags = OFN_EXPLORER | OFN_HIDEREADONLY |
		OFN_ENABLETEMPLATE | OFN_ENABLEHOOK |
		OFN_OVERWRITEPROMPT;
	file.lpTemplateName = MAKEINTRESOURCE(IDD_CHOOSE_TAP_VERSION);
	file.lpstrFile = output_filename;
	file.hInstance = instance;
	file.lpfnHook = tap_save_hook_proc;
	file.lpstrDefExt = "tap";
	if (GetSaveFileName(&file) == FALSE)
		return;

	status_window=CreateDialog(instance,MAKEINTRESOURCE(IDD_STATUS),hwnd,status_window_proc);
	EnableWindow(hwnd, FALSE);
	ShowWindow(status_window, SW_SHOWNORMAL);
	UpdateWindow(status_window);
	_snprintf(msg_string, 128, "Origin: %s",(IsDlgButtonChecked(hwnd, IDC_FROM_WAV) ? input_filename : "sound card"));
	control=GetDlgItem(status_window, IDC_ORIGIN);
	SetWindowText(control, msg_string);
	_snprintf(msg_string, 128, "Destination: %s", output_filename);
	control=GetDlgItem(status_window, IDC_DESTINATION);
	SetWindowText(control, msg_string);
	strncpy(msg_string, IsDlgButtonChecked(hwnd, IDC_FROM_WAV) ? "Progress indication" : "Volume level", 128);
	control=GetDlgItem(status_window, IDC_WHAT_PROGRESSBAR_MEANS);
	SetWindowText(control, msg_string);

	adv=(struct audiotap_advanced *)GetWindowLong(hwnd, GWL_USERDATA);

	params.input_filename = IsDlgButtonChecked(hwnd, IDC_FROM_WAV) ? input_filename : NULL;
	params.output_filename = output_filename;
	params.min_duration = (adv != NULL ? adv->min_duration : 3);
	params.min_height = (adv != NULL ? adv->min_height<<24 : 10<<24);
	params.freq = (adv != NULL ? adv->infreq : 44100);
	params.inverted = IsDlgButtonChecked(hwnd, IDC_TO_TAP_INVERTED) == BST_CHECKED;
	params.tap_version = (adv != NULL ? (unsigned char)adv->tap_version : 1);
	params.clock = (adv != NULL ? adv->clock : 0);

	thread=CreateThread(NULL, 0, audio2tap_thread, &params, 0, &thread_id);

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

	EnableWindow(hwnd, TRUE);
	DestroyWindow(status_window);
	status_window = NULL;
};

BOOL CALLBACK tap2audio_status_window_proc( HWND hwndDlg,
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
		if (LOWORD(wParam) == IDC_STOP)
			tap2audio_interrupt();
		return 1;
	default:
		return 0;
	}

};

struct tap2audio_parameters {
	char *infile;
	char *outfile;
	int inverted;
	int32_t volume;
	int freq;
};

DWORD WINAPI tap2audio_thread(LPVOID params){
	tap2audio(((struct tap2audio_parameters*)params)->infile,
		((struct tap2audio_parameters*)params)->outfile,
		((struct tap2audio_parameters*)params)->inverted,
		((struct tap2audio_parameters*)params)->volume,
		((struct tap2audio_parameters*)params)->freq);
	return 0;
}

void read_from_tap(HWND hwnd){
	OPENFILENAME file;
	char input_filename[1024];
	char output_filename[1024];
	MSG msg;
	struct tap2audio_parameters params;
	DWORD thread_id;
	HANDLE thread;
	char msg_string[128];
	struct audiotap_advanced *adv;

	input_filename[0]=0;
	output_filename[0]=0;
	memset(&file,0,sizeof(file));
	file.lStructSize = sizeof(file);
	file.hwndOwner = hwnd;
	file.nMaxFile = 1024;

	file.lpstrFilter ="TAP files (*.tap)\0*.tap\0All files\0*.*\0\0";
	file.lpstrDefExt = "tap";
	file.nFilterIndex = 1;
	file.lpstrTitle = "Choose the TAP file to be converted to WAV";
	file.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
	file.lpstrFile = input_filename;
	file.lpfnHook = tap_open_hook_proc;
	if (GetOpenFileName(&file) == FALSE)
		return;

	if (IsDlgButtonChecked(hwnd, IDC_TO_WAV)){
		file.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLEHOOK |
		OFN_OVERWRITEPROMPT;
		file.lpstrFile = output_filename;
		file.lpstrFilter ="WAV files (*.wav)\0*.wav\0All files\0*.*\0\0";
		file.lpstrTitle = "Choose the name of the WAV file to be created";
		file.lpstrDefExt = "wav";
		file.nFilterIndex = 1;
		file.lpfnHook = wav_opensave_hook_proc;
		if (GetSaveFileName(&file) == FALSE)
			return;
	}

	status_window=CreateDialog(instance,MAKEINTRESOURCE(IDD_STATUS),hwnd,tap2audio_status_window_proc);
	EnableWindow(hwnd, FALSE);
	ShowWindow(status_window, SW_SHOWNORMAL);
	UpdateWindow(status_window);
	_snprintf(msg_string, 128, "Origin: %s", input_filename);
	SetWindowText(GetDlgItem(status_window, IDC_ORIGIN), msg_string);
	_snprintf(msg_string, 128, "Destination: %s", (IsDlgButtonChecked(hwnd, IDC_FROM_WAV) ? output_filename : "sound card"));
	SetWindowText(GetDlgItem(status_window, IDC_DESTINATION), msg_string);
	strncpy(msg_string, "Progress indication", 128);
	SetWindowText(GetDlgItem(status_window, IDC_WHAT_PROGRESSBAR_MEANS), msg_string);

	adv=(struct audiotap_advanced *)GetWindowLong(hwnd, GWL_USERDATA);

	params.infile=input_filename;
	params.outfile=IsDlgButtonChecked(hwnd, IDC_TO_WAV) ? output_filename : NULL;
	params.inverted = (IsDlgButtonChecked(hwnd, IDC_FROM_TAP_INVERTED) == BST_CHECKED);
	params.volume=(adv != NULL ? adv->volume<<23 : 254<<23);
	params.freq=(adv != NULL ? adv->outfreq : 44100);

	thread=CreateThread(NULL, 0, tap2audio_thread, &params, 0, &thread_id);

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

	EnableWindow(hwnd, TRUE);
	DestroyWindow(status_window);
	status_window = NULL;
}

BOOL CALLBACK to_tap_advanced_proc(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			struct audiotap_advanced *adv = (struct audiotap_advanced *)lParam;
			SetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_FREQ        ,adv->infreq      ,FALSE);
			SetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_HEIGHT  ,adv->min_height  ,FALSE);
			SetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_DURATION,adv->min_duration,FALSE);
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"C64 PAL");
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"C64 NTSC");
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"VIC20 PAL");
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"VIC20 NTSC");
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"C16 PAL");
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_ADDSTRING,0,(LPARAM)"C16 NTSC");
			SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_SETCURSEL,adv->clock,0);
			return 1;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK){
			struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
			BOOL success;
			adv->min_height   = GetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_HEIGHT    ,&success,FALSE);
			if (!success) adv->min_height = 10;
			adv->min_duration = GetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_MIN_DURATION  ,&success,FALSE);
			if (!success) adv->min_duration = 3;
			adv->infreq         = GetDlgItemInt(hwnd,IDC_TO_TAP_ADVANCED_FREQ          ,&success,FALSE);
			if (!success) adv->infreq = 44100;
			if (adv->min_height < 1) adv->min_height = 1;
			adv->clock=SendMessage(GetDlgItem(hwnd,IDC_CLOCKS),CB_GETCURSEL,0,0);
			if (adv->clock==CB_ERR || adv->clock<0 || adv->clock>5)
				adv->clock=0;
			EndDialog(hwnd,0);
		}
		if (LOWORD(wParam) == IDCANCEL){
			EndDialog(hwnd,0);
		}
	default:
		return 0;
	}
}

BOOL CALLBACK from_tap_advanced_proc(HWND hwnd, // handle of window
UINT uMsg, // message identifier
WPARAM wParam, // first message parameter
LPARAM lParam // second message parameter
){

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			struct audiotap_advanced *adv = (struct audiotap_advanced *)lParam;
			SetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_FREQ  ,adv->outfreq,FALSE);
			SetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_VOLUME,adv->volume ,FALSE);
			return 1;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK){
			struct audiotap_advanced *adv = (struct audiotap_advanced *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
			BOOL success;
			adv->volume  = GetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_VOLUME,&success,FALSE);
			if (!success) adv->volume = 254;
			adv->outfreq = GetDlgItemInt(hwnd,IDC_FROM_TAP_ADVANCED_FREQ  ,&success,FALSE);
			if (!success) adv->outfreq = 44100;
			if (adv->volume < 1) adv->volume = 1;
			EndDialog(hwnd,0);
		}
		if (LOWORD(wParam) == IDCANCEL){
			EndDialog(hwnd,0);
		}
	default:
		return 0;
	}
}

BOOL CALLBACK dialog_control(HWND hwnd, // handle of window
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
		}
		SetWindowLong(hwnd, GWL_USERDATA, lParam);

		CheckRadioButton(hwnd,IDC_FROM_TAP,IDC_TO_TAP,IDC_FROM_TAP);
		if (audiotap_status.pablio_init_status == LIBRARY_OK){
			CheckRadioButton(hwnd,IDC_TO_SOUND,IDC_TO_WAV,IDC_TO_SOUND);
			CheckRadioButton(hwnd,IDC_FROM_SOUND,IDC_FROM_WAV,IDC_FROM_SOUND);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_SOUND), TRUE);
		}
		if (audiotap_status.audiofile_init_status == LIBRARY_OK){
			EnableWindow(GetDlgItem(hwnd,IDC_TO_WAV), TRUE);
			if (audiotap_status.pablio_init_status != LIBRARY_OK){
				CheckRadioButton(hwnd,IDC_TO_SOUND,IDC_TO_WAV,IDC_TO_WAV);
				CheckRadioButton(hwnd,IDC_FROM_SOUND,IDC_FROM_WAV,IDC_FROM_WAV);
			}
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_FROM_TAP){
			EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_ADVANCED), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_INVERTED), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_WAV), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_SOUND), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_ADVANCED), TRUE);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_INVERTED), TRUE);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_WAV), audiotap_status.audiofile_init_status == LIBRARY_OK);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_SOUND), audiotap_status.pablio_init_status == LIBRARY_OK);
		}
		if (LOWORD(wParam) == IDC_TO_TAP){
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_ADVANCED), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_TAP_INVERTED), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_WAV), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_SOUND), FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_ADVANCED), TRUE);
			EnableWindow(GetDlgItem(hwnd,IDC_TO_TAP_INVERTED), TRUE);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_WAV), audiotap_status.audiofile_init_status == LIBRARY_OK);
			EnableWindow(GetDlgItem(hwnd,IDC_FROM_SOUND), audiotap_status.pablio_init_status == LIBRARY_OK);
		}
		if (LOWORD(wParam) == IDOK){
			if (IsDlgButtonChecked(hwnd, IDC_TO_TAP))
				save_to_tap(hwnd);
			else
				read_from_tap(hwnd);
		}
		if (LOWORD(wParam) == IDC_TO_TAP_ADVANCED){
			struct audiotap_advanced adv;
			memcpy(&adv,(void*)GetWindowLong(hwnd,GWL_USERDATA),sizeof(adv));
			DialogBoxParam(instance, MAKEINTRESOURCE(IDD_TO_TAP_ADVANCED), hwnd, to_tap_advanced_proc, (LPARAM)&adv);
		}
		if (LOWORD(wParam) == IDC_FROM_TAP_ADVANCED){
			struct audiotap_advanced adv;
			memcpy(&adv,(void*)GetWindowLong(hwnd,GWL_USERDATA),sizeof(adv));
			DialogBoxParam(instance, MAKEINTRESOURCE(IDD_FROM_TAP_ADVANCED), hwnd, from_tap_advanced_proc, (LPARAM)&adv);
		}
		if (LOWORD(wParam) == IDC_ABOUT){
			DialogBox(instance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, about_proc);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;
		break;
	case WM_DESTROY:
		/* The window is being destroyed, close the application
		 * (the child button gets destroyed automatically). */
		PostQuitMessage (0);
		return 1;
		break;
	default:
		return 0;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow ){
	struct audiotap_advanced adv = {44100, 3, 10, 1, 44100, 254, 0};

	instance = hInstance;
	audiotap_status = audiotap_initialize();
	if (audiotap_status.audiofile_init_status != LIBRARY_OK &&
		audiotap_status.pablio_init_status != LIBRARY_OK){
		MessageBox(0,"Both audiofile.dll and pablio.dll are missing or improperly installed",
			"Cannot start Audiotap",MB_ICONERROR);
		return 0;
	}

	return DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_MAINWINDOW),NULL,dialog_control,(LPARAM)&adv);
}
