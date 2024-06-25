#pragma once

#include "PlatformDetection.h"

#include <array>
#include <cstdint>
#include <string>
#include <format>
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <queue>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <filesystem>


namespace esx {
	#define BYTE(x) x
	#define KIBI(x) x * BYTE(1024)
	#define MIBI(x) x * KIBI(1024)

	typedef signed char I8;
	typedef unsigned char U8;
	typedef signed short I16;
	typedef unsigned short U16;
	typedef signed int I32;
	typedef unsigned int U32;
	typedef signed long long I64;
	typedef unsigned long long U64;
	typedef float F32;
	typedef double F64;

	typedef bool BIT;

	#define ESX_TRUE true
	#define ESX_FALSE false


#ifdef UNICODE
	typedef std::wostream OutputStream;
	typedef std::wofstream FileOutputStream;
	typedef std::wifstream FileInputStream;
	typedef std::wstreambuf StreamBuffer;
	typedef std::wstring String;
	typedef std::wstring_view StringView;
	typedef std::iwstringstream InputStringStream;
	typedef std::wstringstream StringStream;
	typedef wchar_t Char;
	#define ESX_TEXT(x) L##x
	#define ESX_CONSOLE_OUT std::wcout
	#define ESX_TO_STRING(x) std::to_wstring(x)
	#define ESX_MAKE_FORMAT_ARGS(x) std::make_wformat_args(x)
#else
	typedef std::ostream OutputStream;
	typedef std::ofstream FileOutputStream;
	typedef std::ifstream FileInputStream;
	typedef std::streambuf StreamBuffer;
	typedef std::string String;
	typedef std::string_view StringView;
	typedef std::istringstream InputStringStream;
	typedef std::stringstream StringStream;
	typedef char Char;
	#define ESX_TEXT(x) x
	#define ESX_CONSOLE_OUT std::cout
	#define ESX_TO_STRING(x) std::to_string(x)
	#define ESX_MAKE_FORMAT_ARGS(x) std::make_format_args(x)
#endif

	typedef const Char* NativeString;

#define ANSI_COLOR_RED     ESX_TEXT("\x1b[31m")
#define ANSI_COLOR_GREEN   ESX_TEXT("\x1b[32m")
#define ANSI_COLOR_YELLOW  ESX_TEXT("\x1b[33m")
#define ANSI_COLOR_BLUE    ESX_TEXT("\x1b[34m")
#define ANSI_COLOR_MAGENTA ESX_TEXT("\x1b[35m")
#define ANSI_COLOR_CYAN    ESX_TEXT("\x1b[36m")
#define ANSI_COLOR_RESET   ESX_TEXT("\x1b[0m")


	template<typename T>
	using Queue = std::queue<T>;

	template<typename T>
	using Vector = std::vector<T>;

	template<typename T, size_t Size>
	using Array = std::array<T, Size>;

	template<typename K, typename V>
	using OrderedMap = std::map<K, V>;

	template<typename K, typename V>
	using UnorderedMap = std::unordered_map<K, V>;

	template<typename K, typename V>
	using Map = std::map<K, V>;

	template<typename T1, typename T2>
	using Pair = std::pair<T1, T2>;

	template<typename... Args>
	using Tuple = std::tuple<Args...>;

	template<typename T>
	using ScopedPtr = std::unique_ptr<T>;

	template<typename T>
	using SharedPtr = std::shared_ptr<T>;

	template<typename T>
	using Function = std::function<T>;

	template<typename T, typename... ARGS>
	constexpr SharedPtr<T> MakeShared(ARGS&&... args) {
		return std::make_shared<T>(std::forward<ARGS>(args)...);
	}

	template<typename T, typename... ARGS>
	constexpr ScopedPtr<T> MakeScoped(ARGS&&... args) {
		return std::make_unique<T>(std::forward<ARGS>(args)...);
	}

	template<typename... ARGS>
	String FormatString(const StringView view, ARGS&&... args) {
		return std::vformat(view, ESX_MAKE_FORMAT_ARGS(args...));
	}


	struct ShiftRegister {
		U8 Data = 0;
		U8 Size = 0;

		void Set(U8 data) {
			Data = data;
			Size = 8;
		}

		void Push(U8 value) {
			if (Size == 8) return;

			Data >>= 1;
			Data |= (value << 7);
			Size++;
		}

		U8 Pop() {
			if (Size == 0) return (Data & 0x1);

			U8 data = Data & 0x1;
			Data >>= 1;
			Size--;
			return data;
		}
	};


#ifdef ESX_DEBUG
#if defined(ESX_PLATFORM_WINDOWS)
#define ESX_DEBUGBREAK() __debugbreak()
#elif defined(ESX_PLATFORM_LINUX)
#include <signal.h>
#define ESX_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#define ESX_ENABLE_ASSERTS
#else
#define ESX_DEBUGBREAK()
#endif

#define ESX_EXPAND_MACRO(x) x

#define ESX_BIT(x) 1 << x
}