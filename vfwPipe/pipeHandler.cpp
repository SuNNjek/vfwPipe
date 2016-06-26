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
	MessageBox(parent, L"vfwPipe v1.0.0 by Sunner (sunnerlp@gmail.com)", L"About vfwPipe", MB_OK | MB_ICONINFORMATION);
}

void pipeHandler::configDlg(HWND parent) {
	DialogBoxParamW(inst, MAKEINTRESOURCE(IDD_DIALOG1), parent, ::ConfigDialog, (LPARAM)this);
}

void pipeHandler::setToDefault()
{
	wcscpy_s(settings->cmdLineArgs, VFWPIPE_DEFAULT_CMD_ARGS);
	wcscpy_s(settings->encoderPath, VFWPIPE_DEFAULT_ENCODER);
}

INT_PTR CALLBACK pipeHandler::ConfigDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	

	switch (uMsg) {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, cmdLineArgsTxt, this->settings->cmdLineArgs);
		SetDlgItemText(hwndDlg, applicationPathTxt, this->settings->encoderPath);
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, WM_CLOSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hwndDlg, cmdLineArgsTxt, this->settings->cmdLineArgs, 2048);
			GetDlgItemText(hwndDlg, applicationPathTxt, this->settings->encoderPath, MAX_PATH);
			EndDialog(hwndDlg, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);
			break;
		case FileChooserBtn:
			{
				wchar_t tmpFilepath[MAX_PATH];

				OPENFILENAMEW ofn;
				ZeroMemory(tmpFilepath, MAX_PATH * sizeof(wchar_t));
				ZeroMemory(&ofn, sizeof(OPENFILENAMEW));

				ofn.lStructSize = sizeof(OPENFILENAMEW);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = L".exe-Dateien\0*.exe\0Alle Dateien\0*.*";
				ofn.lpstrFile = tmpFilepath;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = L"Encoder ausw�hlen";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

				if (GetOpenFileName(&ofn)) {
					SetDlgItemText(hwndDlg, applicationPathTxt, tmpFilepath);
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}



LRESULT pipeHandler::sendToStdout(ICCOMPRESS * icc, size_t icc_size)
{
	//wchar_t* buffer1 = (wchar_t*)malloc(sizeof(wchar_t) * 1024);
	//wsprintf(buffer1, L"this->pipe = %p", this->pipe);
	//MessageBox(0, buffer1, L"lol", 0);
	//free(buffer1);
	
	//assert(this->pipe != nullptr);
	
	LONG size = icc->lpbiInput->biSizeImage;
	if (size < 0) {
		closePipe();
		return ICERR_BADFORMAT;
	}

	icc->lpbiOutput->biSizeImage = LOG_SIZE;
	*icc->lpdwFlags = AVIIF_KEYFRAME;

//#ifdef _DEBUG
//	HANDLE bitmapOut = INVALID_HANDLE_VALUE;
//	std::wostringstream strStream;
//	strStream << Helper::replaceEnvVars(L"%USERPROFILE%\\Bitmap") << this->framesCompressed++ << L".bmp";
//
//	bitmapOut = CreateFile(Helper::replaceEnvVars(L"%USERPROFILE%\\vfwPipeStdErr.txt").c_str(),
//		FILE_APPEND_DATA,
//		FILE_SHARE_WRITE | FILE_SHARE_READ,
//		NULL,
//		CREATE_ALWAYS,
//		FILE_ATTRIBUTE_NORMAL,
//		NULL);
//
//	if (bitmapOut != INVALID_HANDLE_VALUE) {
//		WriteFile(bitmapOut, icc->lpInput, size, NULL, NULL);
//
//		CloseHandle(bitmapOut);
//	}
//#endif

	DWORD bytesWritten;

	if (!WriteFile(this->pipe, icc->lpInput, size, &bytesWritten, NULL)) {
		DWORD dwErr = 0;
		if (!GetExitCodeProcess(this->hProc, &dwErr) || dwErr != STILL_ACTIVE) {
			dwErr = GetLastError();
		}

		_com_error error(dwErr);
		std::wstring msg(error.ErrorMessage());

#ifdef _DEBUG
		Logger::Write(Logger::ERR, Helper::ws2s(msg).c_str());
#endif

		MessageBox(0, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);

		closePipe();

		return ICERR_ERROR;
	}

	wchar_t* buffer = (wchar_t*)malloc(LOG_SIZE);
	wsprintf(buffer, L"Frame %d written, %d Bytes written", icc->lFrameNum, bytesWritten);

	memcpy_s(icc->lpOutput, LOG_SIZE, buffer, LOG_SIZE);

	free(buffer);

	return ICERR_OK;
}

LRESULT pipeHandler::establishPipe()
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

		if (!CreatePipe(&c_in, &this->pipe, &sa, 10 * 1024 * 1024)) {
			throw L"Couldn't create input pipe";
		}

		if (!SetHandleInformation(this->pipe, HANDLE_FLAG_INHERIT, 0)) {
			throw L"Couldn't set input handle information";
		}

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

		std::wostringstream cmdLineStream;
		cmdLineStream << L"\"" << this->settings->encoderPath << L"\"";
		cmdLineStream << L" " << this->settings->cmdLineArgs;
		wchar_t tmpBuffer[2048 + MAX_PATH];
		wcscpy_s(tmpBuffer, cmdLineStream.str().c_str());

		if (!CreateProcess(
			NULL,
			tmpBuffer,
			NULL,
			NULL,
			TRUE,
			CREATE_NO_WINDOW,
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

#ifdef _DEBUG
		std::wostringstream strStream;
		strStream << L"Established Pipe with cmdLine \"" << tmpBuffer << L"\"";
		Logger::Write(Logger::INFO, Helper::ws2s(strStream.str()).c_str());
#endif

		return ICERR_OK;
	}
	catch (std::wstring msg) {
		if (this->pipe != INVALID_HANDLE_VALUE)
			CloseHandle(this->pipe);

		if (this->f_out != INVALID_HANDLE_VALUE)
			CloseHandle(this->f_out);

		if (this->f_err != INVALID_HANDLE_VALUE)
			CloseHandle(this->f_err);

		MessageBox(0, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);

#ifdef _DEBUG
		Logger::Write(Logger::ERR, Helper::ws2s(msg).c_str());
#endif
		return ICERR_BADFORMAT;
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
	Logger::Write(Logger::INFO, strStream.str());
#endif

	return ICERR_OK;
}

LRESULT pipeHandler::getSize(BITMAPINFO * in, BITMAPINFO * out)
{
	return in->bmiHeader.biWidth * in->bmiHeader.biHeight * 3;
}

LRESULT pipeHandler::closePipe()
{
	if (this->hThread == INVALID_HANDLE_VALUE)
		return ICERR_OK;

	CloseHandle(this->pipe);
	CloseHandle(this->f_out);
	CloseHandle(this->f_err);

	while (WaitForSingleObject(this->hProc, INFINITE));
	CloseHandle(this->hProc);
	CloseHandle(this->hThread);


	this->pipe = NULL;
	this->hProc = this->hThread = NULL;

	return ICERR_OK;
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

INT_PTR CALLBACK ConfigDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	pipeHandler *ph = (pipeHandler*)GetWindowLong(hwndDlg, DWL_USER);
	
	if (uMsg == WM_INITDIALOG) {
		ph = (pipeHandler*)lParam;
		SetWindowLong(hwndDlg, DWLP_USER, (LONG)ph);
	}
	if (ph != NULL)
		return ph->ConfigDialog(hwndDlg, uMsg, wParam, lParam);

	return FALSE;
}