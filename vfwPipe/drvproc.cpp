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

#include <Windows.h>
#include <Vfw.h>
#include <sstream>
#include "pipeHandler.h"
#include "Logger.h"


HINSTANCE inst;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		inst = (HINSTANCE)hModule;
		break;
	default:
		break;
	}
	return TRUE;
}

LRESULT WINAPI DriverProc(DWORD_PTR dwDriverId, HDRVR hDriver, UINT uMsg, LONG lParam1, LONG lParam2)
{
	pipeHandler* ph = (pipeHandler*)dwDriverId;

	switch (uMsg) {
	case DRV_LOAD:
		LOG_START;
		return DRV_OK;
	case DRV_FREE:
		LOG_STOP;
		return DRV_OK;

	case DRV_OPEN:
	{
		ICOPEN *ico = (ICOPEN*)lParam2;

		if (ico && ico->fccType != ICTYPE_VIDEO)
			return 0;

		ph = new pipeHandler();
		if (ico)
			ico->dwError = ICERR_OK;
		return (LRESULT)ph;
	}

	case DRV_CLOSE:
		delete ph;
		ph = NULL;
		return DRV_OK;

	case DRV_DISABLE:
	case DRV_ENABLE:
		return (LRESULT)1L;

	case DRV_INSTALL:
	case DRV_REMOVE:
		return (LRESULT)DRV_OK;

	case DRV_QUERYCONFIGURE:    // configuration from drivers applet
		return (LRESULT)0L;

	case DRV_CONFIGURE:
		//return DRV_OK;
		return DRV_CANCEL;

	case ICM_GETINFO:
	{
		ICINFO* icinfo = (ICINFO *)lParam1;

		if (lParam2 < sizeof(ICINFO))
			return 0;

		icinfo->dwSize = sizeof(ICINFO);
		icinfo->fccType = FOURCC_VIDC;
		icinfo->fccHandler = FOURCC_PIPE;
		icinfo->dwFlags = VIDCF_TEMPORAL | VIDCF_FASTTEMPORALC;
		icinfo->dwVersion = 0x00010000;
		icinfo->dwVersionICM = ICVERSION;

		memcpy(icinfo->szName, VFWPIPE_NAME, sizeof(VFWPIPE_NAME));
		memcpy(icinfo->szDescription, VFWPIPE_DESC, sizeof(VFWPIPE_DESC));

		return sizeof(ICINFO);
	}

	case ICM_ABOUT:
		if (lParam1 != -1)
			ph->aboutDlg((HWND)lParam1);
		return ICERR_OK;
	
	case ICM_CONFIGURE:
		if (lParam1 != -1)
			ph->configDlg((HWND)lParam1);
		return ICERR_OK;

	case ICM_GETSTATE:
		if (lParam2 == NULL)
			return sizeof(pipeSettings);
		else {
			if (sizeof(pipeSettings) >= lParam2) {
				memcpy((void*)lParam1, ph->settings, lParam2);
			}
			else {
				memcpy((void*)lParam1, ph->settings, sizeof(pipeSettings));
			}
		}
		return ICERR_OK;

	case ICM_SETSTATE:
		if (lParam1 == NULL)
			ph->setToDefault();
		memcpy((void*)ph->settings, (void*)lParam1, sizeof(pipeSettings));
		return ICERR_OK;

	case ICM_GET:
		if (!(void *)lParam1)
			return 0;
		return ICERR_OK;

	case ICM_SET:
		return 0;

	case ICM_GETDEFAULTQUALITY:
		if (lParam1) {
			*((LPDWORD)lParam1) = 10000;
			return ICERR_OK;
		}
		break;

	case ICM_GETQUALITY:
	case ICM_SETQUALITY:
	case ICM_GETBUFFERSWANTED:
	case ICM_GETDEFAULTKEYFRAMERATE:
		return ICERR_UNSUPPORTED;

	case ICM_COMPRESS:
		return ph->sendToStdout((ICCOMPRESS*)lParam1, (size_t)lParam2);

	case ICM_COMPRESS_QUERY:
		{
			BITMAPINFO* bmi = (BITMAPINFO*)lParam1;
			if (ph->getImageSize(&bmi->bmiHeader) == -1)
				return ICERR_BADFORMAT;
			else
				return ICERR_OK;
		}

	case ICM_COMPRESS_BEGIN:
		if (!ph->checkFormat((LPBITMAPINFO)lParam1))
			return ICERR_BADFORMAT;

		return ph->establishPipe((LPBITMAPINFO)lParam1);

	case ICM_COMPRESS_END:
	{
		return ph->closePipe();
	}
		
	case ICM_COMPRESS_GET_FORMAT:
		return ph->getFormat((BITMAPINFO*)lParam1, (BITMAPINFO*)lParam2);

	case ICM_COMPRESS_GET_SIZE:
		//return ph->getSize((BITMAPINFO*)lParam1, (BITMAPINFO*)lParam2);
		return LOG_SIZE;

	case ICM_COMPRESS_FRAMES_INFO:
		return ICERR_OK;

	case ICM_DECOMPRESS:
	case ICM_DECOMPRESS_QUERY:
	case ICM_DECOMPRESS_GET_FORMAT:
	case ICM_DECOMPRESS_BEGIN:
	case ICM_DECOMPRESS_END:
		return ICERR_UNSUPPORTED;

	default:
		if (uMsg < DRV_USER)
			return DefDriverProc(dwDriverId, hDriver, uMsg, lParam1, lParam2);
		else
			return ICERR_UNSUPPORTED;
	}
	return ICERR_UNSUPPORTED;
}
