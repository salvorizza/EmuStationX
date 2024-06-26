#pragma once

#include "Base.h"
#include "Utils/LoggingSystem.h"

#ifdef ESX_ENABLE_ASSERTS
	#define ESX_ASSERT(condition,message,...) if(!(condition)) { ESX_CORE_LOG_ERROR(message,__VA_ARGS__); ESX_DEBUGBREAK(); }
	#define ESX_CORE_ASSERT(condition,message,...) if(!(condition)) { ESX_CORE_LOG_ERROR(message,__VA_ARGS__); ESX_DEBUGBREAK(); }
#else
	#define ESX_ASSERT(condition,message,...) if(!(condition)) { ESX_CORE_LOG_ERROR(message,__VA_ARGS__); }
	#define ESX_CORE_ASSERT(condition,message,...) if(!(condition)) { ESX_CORE_LOG_ERROR(message,__VA_ARGS__); }
#endif

#define ESX_VERIFY(condition,message,...) if(!(condition)) { ESX_CORE_LOG_ERROR(message,__VA_ARGS__); }
#define ESX_CORE_VERIFY(condition,message,...) if(!(condition)) { ESX_CORE_LOG_ERROR(message,__VA_ARGS__); }