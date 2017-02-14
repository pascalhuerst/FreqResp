#pragma once

#include <vector>
#include <string>

class Debug
{
public:
	// Only static stuff here, so object creation is nonsense.
	Debug(Debug const&) = delete;
	Debug& operator=(Debug const&) = delete;

	enum Level {
		LevelNone		= 0,
		LevelError		= 1,
		LevelWarning	= 2,
		LevelDebug		= 3,
		LevelVerbose	= 4
	};

	Level getDebugLevel();
	void setDebugLevel(Level l);

	static void error(const std::string& name, const std::string& msg);
	static void warning(const std::string& name, const std::string& msg);
	static void debug(const std::string& name, const std::string& msg);
	static void verbose(const std::string& name, const std::string& msg);

private:
	static Level s_debugLevel;

	static void write(Level level, const std::string& name, const std::string& msg);

	const static std::vector<std::string> s_debugLevelNames;
};

