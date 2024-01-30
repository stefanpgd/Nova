#pragma once
// Credits for this logger util go to: Matěj Kaločai
// Github: https://github.com/WhatevvsDev

#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#endif
#include <Windows.h>

#define LOG_IN_RELEASE true

namespace Log
{
	enum class MessageType
	{
		Default,
		Debug,
		Error
	};

	namespace
	{
		// Get Windows Console specific attribute for different text color
		WORD type_to_color(MessageType aType)
		{
			switch(aType)
			{
			default:
			case MessageType::Default:
				return 8;
			case MessageType::Debug:
				return 14;
			case MessageType::Error:
				return 12;
			}
		}
	}

	inline void print(MessageType aType, const char* aFile, int aLineNumber, const std::string& aMessage)
	{
#if _DEBUG || LOG_IN_RELEASE
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

		// Modify color of text
		WORD attribute = type_to_color(aType);
		SetConsoleTextAttribute(handle, attribute);

		// Get only file name
		std::string fileName{ aFile };
		fileName = fileName.substr(fileName.find_last_of("/\\") + 1).c_str();

		// Print and reset color
		printf("[%s: %i] - %s\n", fileName.c_str(), aLineNumber, aMessage.c_str());
		SetConsoleTextAttribute(handle, 15);
#endif
	}
}

#define LOGMSG_2(type, message)			Log::print(type, __FILE__, __LINE__, message);
#define LOGMSG_1(message)				LOGMSG_2(Log::MessageType::Default, message)

#define FUNC_CHOOSER(_f1, _f2, _f3, ...) _f3
#define FUNC_RECOMPOSER(argsWithParentheses) FUNC_CHOOSER argsWithParentheses
#define MACRO_CHOOSER(...) FUNC_RECOMPOSER((__VA_ARGS__, LOGMSG_2, LOGMSG_1, ))
#define LOG(...) MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)