#pragma once

#include <string>
#include <Windows.h>

#define ENV_VARS_BUFFER_SIZE MAX_PATH

class Helper
{
public:
	// multi byte to wide char:
	static std::wstring s2ws(const std::string& str);

	// wide char to multi byte:
	static std::string ws2s(const std::wstring& wstr);

	static std::wstring replaceEnvVars(std::wstring path);
};

