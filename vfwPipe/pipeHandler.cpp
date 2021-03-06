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

#include "pipeHandler.h"
#include "resource.h"
#include "Logger.h"
#include "Helper.h"

extern HINSTANCE inst;

pipeHandler::pipeHandler()
{
	settings = (pipeSettings*)malloc(sizeof(pipeSettings));
	vFormat = (vidFormat*)malloc(sizeof(vidFormat));
	setToDefault();
}

pipeHandler::~pipeHandler()
{
	free(settings);
	free(vFormat);
	closePipe();
}

void pipeHandler::aboutDlg(HWND parent) {
	std::wostringstream stream;
	stream << L"vfwPipe version " << VFWPIPE_VERSION_STRING_WITH_HASH << L"\nBy " << VFWPIPE_AUTHOR_STRING;
	MessageBox(parent, stream.str().c_str(), L"About vfwPipe", MB_OK | MB_ICONINFORMATION);
}

void pipeHandler::configDlg(HWND parent) {
	DialogBoxParam(inst, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), parent, ::ConfigDialog, (LPARAM)this);
}

void pipeHandler::setToDefault()
{
	ZeroMemory(this->settings, sizeof(pipeSettings));

	wcscpy_s(settings->cmdLineArgs, VFWPIPE_DEFAULT_CMD_ARGS);
	wcscpy_s(settings->encoderPath, VFWPIPE_DEFAULT_ENCODER);
	wcscpy_s(settings->encoderPath, L"ffmpeg.exe");
	wcscpy_s(settings->outputPath, L"output.mkv");
	settings->openNewWindow = true;
}

INT_PTR CALLBACK pipeHandler::ConfigDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, cmdLineArgsTxt, this->settings->cmdLineArgs);
		SetDlgItemText(hwndDlg, applicationPathTxt, this->settings->encoderPath);
		SetDlgItemText(hwndDlg, outputPathTxt, this->settings->outputPath);
		CheckDlgButton(hwndDlg, openInNewWndwBox, (this->settings->openNewWindow) ? BST_CHECKED : BST_UNCHECKED);
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, WM_CLOSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				GetDlgItemText(hwndDlg, cmdLineArgsTxt, this->settings->cmdLineArgs, 2048);
				GetDlgItemText(hwndDlg, applicationPathTxt, this->settings->encoderPath, MAX_PATH);
				GetDlgItemText(hwndDlg, outputPathTxt, this->settings->outputPath, MAX_PATH);

				UINT chckBoxState = IsDlgButtonChecked(hwndDlg, openInNewWndwBox);
				if (chckBoxState == BST_CHECKED)
					this->settings->openNewWindow = true;
				else
					this->settings->openNewWindow = false;

				EndDialog(hwndDlg, IDOK);
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);
			return TRUE;
		case EncoderChooserBtn:
			{
				wchar_t tmpFilepath[MAX_PATH];

				ZeroMemory(tmpFilepath, MAX_PATH * sizeof(wchar_t));

				GetDlgItemText(hwndDlg, applicationPathTxt, tmpFilepath, MAX_PATH);

				OPENFILENAMEW ofn;
				ZeroMemory(&ofn, sizeof(OPENFILENAMEW));

				ofn.lStructSize = sizeof(OPENFILENAMEW);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = L".exe files\0*.exe\0All files\0*.*\0\0";
				ofn.lpstrFile = tmpFilepath;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = L"Choose encoder";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

				if (GetOpenFileName(&ofn)) {
					SetDlgItemText(hwndDlg, applicationPathTxt, tmpFilepath);
				}
			}
			return TRUE;
		case outfileBtn:
			{
				wchar_t tmpFilepath[MAX_PATH];
				ZeroMemory(tmpFilepath, MAX_PATH * sizeof(wchar_t));

				GetDlgItemText(hwndDlg, outputPathTxt, tmpFilepath, MAX_PATH);

				OPENFILENAMEW ofn;
				ZeroMemory(&ofn, sizeof(OPENFILENAMEW));

				ofn.lStructSize = sizeof(OPENFILENAMEW);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = L"Matroska files\0*.mkv\0MPEG-4 files\0*.mp4\0All Files\0*.*\0\0";
				ofn.lpstrDefExt = L"mkv";
				ofn.lpstrFile = tmpFilepath;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = L"Output file";
				ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

				if (GetSaveFileName(&ofn)) {
					SetDlgItemText(hwndDlg, outputPathTxt, tmpFilepath);
				}
			}
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

LRESULT pipeHandler::sendToStdout(ICCOMPRESS * icc, size_t icc_size)
{
	LONG size = icc->lpbiInput->biSizeImage;
	if (size < 0) {
		return ICERR_BADFORMAT;
	}

	icc->lpbiOutput->biSizeImage = LOG_SIZE;
	*icc->lpdwFlags = AVIIF_KEYFRAME;

	BYTE* newInput = bmpToRGB((BYTE*)icc->lpInput, icc->lpbiInput->biWidth, icc->lpbiInput->biHeight);

	DWORD bytesWritten;

	if (!WriteFile(this->writePipe, newInput, size, &bytesWritten, NULL)) {
		DWORD dwErr = 0;
		if (!GetExitCodeProcess(this->hProc, &dwErr) || dwErr != STILL_ACTIVE) {
			dwErr = GetLastError();
		}

		_com_error error(dwErr);
		std::wstring msg(error.ErrorMessage());

		LOG_ERROR(Helper::ws2s(msg).c_str());

		MessageBox(0, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);

		closePipe();
		free(newInput);

		return ICERR_ERROR;
	}

	free(newInput);

	char* buffer = (char*)malloc(LOG_SIZE);
	sprintf_s(buffer, LOG_SIZE, "Frame %d written, %d Bytes written", icc->lFrameNum, bytesWritten);

	memcpy_s(icc->lpOutput, LOG_SIZE, buffer, LOG_SIZE);

	free(buffer);

	return ICERR_OK;
}

LRESULT pipeHandler::establishPipe(LPBITMAPINFO info)
{
	HANDLE c_in = NULL;

	this->hProc = NULL;
	this->hThread = NULL;

	try {
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		SECURITY_ATTRIBUTES sa;

		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = NULL;

		if (!CreatePipe(&c_in, &this->writePipe, &sa, 10 * 1024 * 1024))
			throw L"Couldn't create input pipe";

		if (!SetHandleInformation(this->writePipe, HANDLE_FLAG_INHERIT, 0))
			throw L"Couldn't set input handle information";

		this->f_out = CreateFile(Helper::replaceEnvVars(L"%USERPROFILE%\\vfwPipeStdOut.txt").c_str(),
			FILE_APPEND_DATA,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			&sa,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		this->f_err = CreateFile(Helper::replaceEnvVars(L"%USERPROFILE%\\vfwPipeStdErr.txt").c_str(),
			FILE_APPEND_DATA,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			&sa,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		memset(&si, 0, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.hStdInput = c_in;
		si.hStdOutput = f_out;
		si.hStdError = f_err;
		si.dwFlags = STARTF_USESTDHANDLES;

		std::wstring argsString(this->settings->cmdLineArgs);
		replaceSubstring(argsString, L"[[width]]", std::to_wstring(info->bmiHeader.biWidth));
		replaceSubstring(argsString, L"[[height]]", std::to_wstring(info->bmiHeader.biHeight));
		replaceSubstring(argsString, L"[[output]]", this->settings->outputPath);

		std::wostringstream cmdLineStream;
		cmdLineStream << L"\"" << this->settings->encoderPath << L"\"";
		cmdLineStream << L" " << argsString;
		wchar_t tmpBuffer[2048 + MAX_PATH];
		wcscpy_s(tmpBuffer, cmdLineStream.str().c_str());

		DWORD creationFlags = (this->settings->openNewWindow) ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;

		if (!CreateProcess(
			NULL,
			tmpBuffer,
			NULL,
			NULL,
			TRUE,
			creationFlags,
			NULL,
			NULL,
			&si,
			&pi
			))
		{
			DWORD dwErr = 0;
			dwErr = GetLastError();
			CloseHandle(pi.hProcess);

			_com_error error(dwErr);
			throw Helper::ws2s(error.ErrorMessage());
		}


		this->hProc = pi.hProcess;
		this->hThread = pi.hThread;

		CloseHandle(c_in);

		std::wostringstream strStream;
		strStream << L"Established Pipe with cmdLine \"" << tmpBuffer << L"\"";
		LOG_INFO(Helper::ws2s(strStream.str()).c_str());

		return ICERR_OK;
	}
	catch (std::wstring msg) {	
		if (this->writePipe != INVALID_HANDLE_VALUE)
			CloseHandle(this->writePipe);

		if (this->f_out != INVALID_HANDLE_VALUE)
			CloseHandle(this->f_out);

		if (this->f_err != INVALID_HANDLE_VALUE)
			CloseHandle(this->f_err);

		MessageBox(0, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);

		LOG_ERROR(Helper::ws2s(msg).c_str());

		return ICERR_ERROR;
	}
}

void pipeHandler::setFormat(ICCOMPRESSFRAMES * iccf)
{
	vFormat->height = iccf->lpbiInput->biHeight;
	vFormat->width = iccf->lpbiInput->biWidth;
	vFormat->numFrames = iccf->lFrameCount;
	vFormat->fps = iccf->dwRate / iccf->dwScale;
	vFormat->quality = iccf->lQuality;
}

LRESULT pipeHandler::getFormat(BITMAPINFO * in, BITMAPINFO * out)
{
	if (out == NULL)
		return sizeof(BITMAPINFO);
	memcpy(out, in, sizeof(BITMAPINFO));
	out->bmiHeader.biCompression = FOURCC_PIPE;

#ifdef _DEBUG
	std::ostringstream strStream;
	strStream << "BitCount: " << in->bmiHeader.biBitCount
		<< " Dimensions: " << in->bmiHeader.biWidth << "x" << in->bmiHeader.biHeight;
	LOG_DEBUG(strStream.str());
#endif

	return ICERR_OK;
}

LRESULT pipeHandler::getSize(BITMAPINFO * in, BITMAPINFO * out)
{
	return in->bmiHeader.biWidth * in->bmiHeader.biHeight * 3;
}

LRESULT pipeHandler::closePipe()
{
	try {
		if(this->writePipe != INVALID_HANDLE_VALUE)
			CloseHandle(this->writePipe);
		if(this->f_out != INVALID_HANDLE_VALUE)
			CloseHandle(this->f_out);
		if(this->f_err != INVALID_HANDLE_VALUE)
			CloseHandle(this->f_err);

		if (this->hProc != INVALID_HANDLE_VALUE) {
			DWORD exitCode;
			do {
				GetExitCodeProcess(this->hProc, &exitCode);
			} while (exitCode == STILL_ACTIVE);
		}

		if (this->hThread != INVALID_HANDLE_VALUE) {
			TerminateThread(this->hThread, 0);
			CloseHandle(this->hThread);
		}
	
		this->writePipe = INVALID_HANDLE_VALUE;
		this->hProc = this->hThread = INVALID_HANDLE_VALUE;
	}
	catch (...) {
		return ICERR_OK;
	}

	return ICERR_OK;
}

bool pipeHandler::checkFormat(LPBITMAPINFO inFormat)
{
	if (inFormat->bmiHeader.biBitCount != 24 || inFormat->bmiHeader.biCompression != BI_RGB)
		return false;

	return true;
}

LONG pipeHandler::getImageSize(BITMAPINFOHEADER * bmi)
{
	if (bmi->biSizeImage)
		return bmi->biSizeImage;
	if (bmi->biCompression == BI_RGB)
	{
		int pitch = bmi->biWidth * bmi->biBitCount / 8;
		// adjust to mod 4
		pitch = (pitch + 3) & ~3;
		// height < 0 means vflip, so strip it here
		if (bmi->biHeight < 0)
			return pitch * -bmi->biHeight;
		else
			return pitch * bmi->biHeight;
	}
	// only BI_RGB allows non-specified size?
	return -1;
}

//REMEMBER TO FREE THE ALLOCATED BUFFER AFTERWARDS TO AVOID MEMORY LEAKS
BYTE * pipeHandler::bmpToRGB(BYTE * buffer, DWORD width, DWORD height)
{
	if (buffer == NULL || width == 0 || height == 0)
		return NULL;

	//Calculate stride assuming bit depth of 24 bpp
	DWORD stride = ((((width * 24) + 31) & ~31) >> 3);

	BYTE* newBuffer = (BYTE*)malloc(3 * width * height * sizeof(BYTE));

	DWORD bufpos = 0;
	DWORD newpos = 0;
	for (DWORD y = 0; y < height; y++) {
		for (DWORD x = 0; x < 3 * width; x += 3)
		{
			newpos = y * 3 * width + x;
			bufpos = (height - y - 1) * stride + x;

			newBuffer[newpos] = buffer[bufpos + 2];
			newBuffer[newpos + 1] = buffer[bufpos + 1];
			newBuffer[newpos + 2] = buffer[bufpos];
		}
	}

	return newBuffer;
}

void pipeHandler::replaceSubstring(std::wstring &input, std::wstring toReplace, std::wstring replacement)
{
	if (toReplace.empty() || input.empty())
		return;

	size_t currPos = 0;
	while ((currPos = input.find(toReplace, currPos)) != std::wstring::npos)
	{
		input.replace(currPos, toReplace.length(), replacement);
		currPos += replacement.length();
	}
}

INT_PTR CALLBACK ConfigDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	pipeHandler *ph = (pipeHandler*)GetWindowLongPtr(hwndDlg, DWLP_USER);
	
	if (uMsg == WM_INITDIALOG) {
		ph = (pipeHandler*)lParam;
		SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)ph);
	}
	if (ph != NULL)
		return ph->ConfigDialog(hwndDlg, uMsg, wParam, lParam);

	return FALSE;
}
