#include <string.h>
#include <sstream>

#include "descriptiveexception.h"

DescriptiveException::DescriptiveException(const char* func, const char* file, int line, int errorNumber, const char* what) :
m_line(line),
m_errno(errorNumber)
{
	size_t len = strlen(what);
	size_t s = s_msgLength > len ? len : s_msgLength;
	memcpy(m_msg, what, s);
	m_msg[s] = 0;

	len = strlen(func);
	s = s_funcNameLength > len ? len : s_funcNameLength;
	memcpy(m_funcName, func, s);
	m_funcName[s] = 0;

	len = strlen(file);
	s = s_fileNameLength > len ? len : s_fileNameLength;
	memcpy(m_fileName, file, s);
	m_fileName[s] = 0;
}

const char* DescriptiveException::what() const noexcept
{
	return m_msg;
}

const char* DescriptiveException::file() const noexcept
{
	return m_fileName;
}

const char* DescriptiveException::func() const noexcept
{
	return m_funcName;
}

int DescriptiveException::line() const noexcept
{
	return m_line;
}

int DescriptiveException::errorNumber() const noexcept
{
	return m_errno;
}

std::string where(const DescriptiveException& de)
{
	std::stringstream ret;
	ret << "File     :" << de.file() << std::endl
	<< "Function :" << de.func() << std::endl
	<< "Line     :" << de.line() << std::endl;

	return ret.str();
}
