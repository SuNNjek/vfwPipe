// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#include <Windows.h>
#include <Vfw.h>
#include <sstream>
#include "pipeHandler.h"
#include "Logger.h"


HINSTANCE inst;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
#ifdef _DEBUG
	std::ostringstream strStream;
#endif

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		strStream << "Dll was attached to process at address " << hModule;
		Logger::Write(Logger::INFO, strStream.str());
#endif
		inst = (HINSTANCE)hModule;
		break;
#ifdef _DEBUG
	case DLL_PROCESS_DETACH:
		strStream << "Dll (at address " << inst << ") was detached from process";
		Logger::Write(Logger::INFO, "Dll  was detached from process");
		break;
#endif
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
#ifdef _DEBUG
		Logger::Start(Logger::DEBUG, "D:\\vfwPipeLog.txt");
		Logger::Write(Logger::INFO, "DRV_LOAD");
#endif
		return DRV_OK;
	case DRV_FREE:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "DRV_CLOSE");
		Logger::Stop();
#endif
		return DRV_OK;

	case DRV_OPEN:
	{
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "DRV_OPEN");
#endif
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
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_GETINFO");
#endif
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
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_ABOUT");
#endif
		if (lParam1 != -1)
			ph->aboutDlg((HWND)lParam1);
		return ICERR_OK;
	
	case ICM_CONFIGURE:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_CONFIGURE");
#endif
		if (lParam1 != -1)
			ph->configDlg((HWND)lParam1);
		return ICERR_OK;

	case ICM_GETSTATE:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_GETSTATE");
#endif
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
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_SETSTATE");
#endif
		if (lParam1 == NULL)
			ph->setToDefault();
		memcpy((void*)ph->settings, (void*)lParam1, sizeof(pipeSettings));
		return ICERR_OK;

	case ICM_GET:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_GET");
#endif
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
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS");
#endif
		return ph->sendToStdout((ICCOMPRESS*)lParam1, (size_t)lParam2);

	case ICM_COMPRESS_QUERY:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS_QUERY");
#endif
		{
			BITMAPINFO* bmi = (BITMAPINFO*)lParam1;
			if (ph->getImageSize(&bmi->bmiHeader) == -1)
				return ICERR_BADFORMAT;
			else
				return ICERR_OK;
		}

	case ICM_COMPRESS_BEGIN:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS_BEGIN");
#endif
		return ph->establishPipe();

	case ICM_COMPRESS_END:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS_END");
#endif
		return ph->closePipe();

	case ICM_COMPRESS_GET_FORMAT:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS_GET_FORMAT");
#endif
		return ph->getFormat((BITMAPINFO*)lParam1, (BITMAPINFO*)lParam2);

	case ICM_COMPRESS_GET_SIZE:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS_GET_SIZE");
#endif
		//return ph->getSize((BITMAPINFO*)lParam1, (BITMAPINFO*)lParam2);
		return LOG_SIZE;

	case ICM_COMPRESS_FRAMES_INFO:
#ifdef _DEBUG
		Logger::Write(Logger::INFO, "ICM_COMPRESS_FRAMES_INFO");
#endif
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
