#include "LoggingSystem.h"
#include "Base/Assert.h"



namespace esx {

	LoggingSpecifications LoggingSystem::sSpecs;
	SharedPtr<Logger> LoggingSystem::sCoreLogger;

	void LoggingSystem::Start(const LoggingSpecifications& specs)
	{
		if (specs.FilePath.empty()) {
			sCoreLogger = MakeShared<ConsoleLogger>(ESX_TEXT("Core"));
		} else {
			sCoreLogger = MakeShared<FileLogger>(ESX_TEXT("Core"),specs.FilePath);
		}
		ESX_CORE_LOG_INFO("Core Logger Initialized");

		ESX_CORE_LOG_INFO("Logging system starting");
	}

	void LoggingSystem::Shutdown()
	{
		ESX_CORE_LOG_INFO("Logging system shutdown");
	}

	Logger::Logger(const StringView& name)
		: mName(name)
	{
	}

	String Logger::FormatLog(const StringView& name, LogType type, const StringView& message, BIT colorMode)
	{
		auto local = std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() };

		StringView color = ANSI_COLOR_MAGENTA;
		StringView stringType;
		switch (type)
		{
			case LogType::Info:
				color = ANSI_COLOR_GREEN;
				stringType = ESX_TEXT("INFO");
				break;

			case LogType::Trace:
				color = ANSI_COLOR_CYAN;
				stringType = ESX_TEXT("TRACE");
				break;

			case LogType::Warning:
				color = ANSI_COLOR_YELLOW;
				stringType = ESX_TEXT("WARNING");
				break;

			case LogType::Error:
				color = ANSI_COLOR_RED;
				stringType = ESX_TEXT("ERROR");
				break;

			case LogType::Fatal:
				color = ANSI_COLOR_RED;
				stringType = ESX_TEXT("FATAL");
				break;
		}

		if (colorMode) {
			return FormatString(ESX_TEXT("{}{} [{}] {} {}{}\n"), color, name, local, stringType, message, ANSI_COLOR_RESET);
		}
		else {
			return FormatString(ESX_TEXT("{} [{}] {} {}\n"), name, local, stringType, message);
		}
	}

	void Logger::LogInternal(OutputStream* stream, LogType type, const StringView& message, BIT colorMode)
	{
		(*stream) << FormatLog(mName, type, message, colorMode);
	}

	ConsoleLogger::ConsoleLogger(const StringView& name)
		:	Logger(name) {
	}

	ConsoleLogger::~ConsoleLogger() {
	}

	void ConsoleLogger::Log(LogType type, const StringView& message) {
		Logger::LogInternal(&ESX_CONSOLE_OUT, type, message);
	}

	FileLogger::FileLogger(const StringView& name, const String& filePath)
		:	Logger(name) 
	{
		mStream = MakeScoped<FileOutputStream>(filePath);
	}

	FileLogger::~FileLogger() {
		mStream->close();
	}

	void FileLogger::Log(LogType type, const StringView& message) {
		Logger::LogInternal(mStream.get(), type, message, ESX_FALSE);
	}

}