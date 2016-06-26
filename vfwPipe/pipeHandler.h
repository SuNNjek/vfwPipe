/**
* vfwPipe, vfw codec which pipes its output to a console application like ffmpeg
*
* Copyright (C) 2016 Robin Heinemeier
*
* This file is part of vfwPipe.
*
* vfwPipe is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License as published by the Free Software Foundation, either version 2 of the
* License, or (at your option) any later version.
*
* vfwPipe is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* See the GNU General Public License for more details. You should have received a copy of the GNU
* General Public License along with vfwPipe. If not, see <http://www.gnu.org/licenses/>.
*
* Authors: Robin Heinemeier
*/

#ifndef __PIPE_HANDLER_H_
#define __PIPE_HANDLER_H_

#pragma comment( lib, "Winmm.lib" )

#include <Windows.h>
#include <Vfw.h>
#include <stdio.h>
#include <io.h>
#include <cstdarg>
#include <comdef.h>
#include <string>
#include <sstream>

#define FOURCC_PIPE mmioFOURCC('P', 'I', 'P', 'E')
#define FOURCC_VIDC mmioFOURCC('V', 'I', 'D', 'C')

#define VFWPIPE_DEFAULT_CMD_ARGS L"-i pipe:0 -f rawvideo -pix_fmt rgb24 -vcodec libx264 \"D:\\test.mkv\""
#define VFWPIPE_DEFAULT_ENCODER L"ffmpeg.exe"
#define VFWPIPE_NAME L"vfwPipe"
#define VFWPIPE_DESC L"vfwPipe v1.0.0"

#define LOG_SIZE 2048
#define BUFFER_SIZE 256

struct pipeSettings {
	wchar_t encoderPath[MAX_PATH];
	wchar_t cmdLineArgs[2048];
};

struct vidFormat {
	int height, width;
	double fps;
	long numFrames, quality;
};

class pipeHandler {
public:

	pipeSettings* settings;

	pipeHandler();
	~pipeHandler();

	//Dialogs
	void aboutDlg(HWND parent);
	void configDlg(HWND parent);

	void setToDefault();
	LRESULT sendToStdout(ICCOMPRESS* icc, size_t icc_size);
	LRESULT establishPipe();
	void setFormat(ICCOMPRESSFRAMES* iccf);
	LRESULT getFormat(BITMAPINFO* in, BITMAPINFO* out);
	LRESULT getSize(BITMAPINFO* in, BITMAPINFO* out);
	LRESULT closePipe();

	LONG getImageSize(BITMAPINFOHEADER* bmi);

	INT_PTR CALLBACK ConfigDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	HANDLE hProc = INVALID_HANDLE_VALUE;
	HANDLE hThread = INVALID_HANDLE_VALUE;
	HANDLE pipe = INVALID_HANDLE_VALUE;

	HANDLE f_out = INVALID_HANDLE_VALUE;
	HANDLE f_err = INVALID_HANDLE_VALUE;
	vidFormat* vFormat;
};

INT_PTR CALLBACK ConfigDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif