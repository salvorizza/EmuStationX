#pragma once

#include "Base/Base.h"

#include <chrono>

namespace esx {

	enum class LogType {
		Info, Trace, Warning, Error, Fatal
	};

	class Logger {
	public:
		Logger(const StringView& name);
		virtual ~Logger() = default;

		template<typename... ARGS>
		void LogFormatted(LogType type, const StringView view, ARGS&&... args) {
			String message = FormatString(view, args...);
			Log(type, message);
		}

		virtual void Log(LogType type, const StringView& message) = 0;

		static String FormatLog(const StringView& name, LogType type, const StringView& message, BIT colorMode);

	protected:
		void LogInternal(OutputStream* stream, LogType type, const StringView& message, BIT colorMode = ESX_TRUE);

	private:
		StringView mName;
	};

	class ConsoleLogger : public Logger {
	public:
		ConsoleLogger(const StringView& name);
		~ConsoleLogger();

		virtual void Log(LogType type, const StringView& message) override;
	};

	class FileLogger : public Logger {
	public:
		FileLogger(const StringView& name, const String& filePath);
		~FileLogger();

		virtual void Log(LogType type, const StringView& message) override;

	private:
		ScopedPtr<FileOutputStream> mStream;
	};

	struct LoggingSpecifications {
		String FilePath;

		LoggingSpecifications() = default;

		LoggingSpecifications(const String& filePath)
			:	FilePath(filePath)
		{}
	};

	class LoggingSystem {
	public:
		LoggingSystem() = delete;
		~LoggingSystem() = delete;

		static void Start(const LoggingSpecifications& specs);
		static void Shutdown();

		static const SharedPtr<Logger>& GetCoreLogger() { return sCoreLogger; }
		static void SetCoreLogger(const SharedPtr<Logger>& logger) { sCoreLogger = logger; }

	private:
		static LoggingSpecifications sSpecs;
		static SharedPtr<Logger> sCoreLogger;
	};

	#define ESX_CORE_LOG(type,x,...) esx::LoggingSystem::GetCoreLogger()->LogFormatted(type, ESX_TEXT(x), __VA_ARGS__) \
			
	#define ESX_CORE_LOG_INFO(x,...) ESX_CORE_LOG(esx::LogType::Info,x,__VA_ARGS__)
	#define ESX_CORE_LOG_TRACE(x,...) ESX_CORE_LOG(esx::LogType::Trace,x,__VA_ARGS__)
	#define ESX_CORE_LOG_WARNING(x,...) ESX_CORE_LOG(esx::LogType::Warning,x,__VA_ARGS__)
	#define ESX_CORE_LOG_ERROR(x,...) ESX_CORE_LOG(esx::LogType::Error,x,__VA_ARGS__)
	#define ESX_CORE_LOG_FATAL(x,...) ESX_CORE_LOG(esx::LogType::Fatal,x,__VA_ARGS__)

}