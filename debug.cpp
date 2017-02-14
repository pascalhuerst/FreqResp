#include "debug.h"

#include <iostream>

const std::vector<std::string> Debug::s_debugLevelNames = {
	"None   ",
	"Error  ",
	"Warning",
	"Debug  ",
	"Verbose"
};

Debug::Level Debug::s_debugLevel = Debug::LevelNone;

// Static
Debug::Level Debug::getDebugLevel()
{
	return Debug::s_debugLevel;
}

// Static
void Debug::setDebugLevel(Debug::Level l)
{
	Debug::s_debugLevel = l;
}

// Static
void Debug::error(const std::string& name, const std::string& msg)
{
	write(LevelError, name, msg);
}

// Static
void Debug::warning(const std::string& name, const std::string& msg)
{
	write(LevelWarning, name, msg);
}

// Static
void Debug::debug(const std::string& name, const std::string& msg)
{
	write(LevelDebug, name, msg);
}

// Static
void Debug::verbose(const std::string& name, const std::string& msg)
{
	write(LevelVerbose, name, msg);
}

// Static
void Debug::write(Level level, const std::string& name, const std::string &msg)
{
	if (level >= s_debugLevel)
		std::cout << s_debugLevelNames[level] << " " << name << " :" << msg << std::endl;
}
