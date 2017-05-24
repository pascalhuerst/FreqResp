#include <iostream>
#include <iterator>
#include <thread>

#include <vector>

#include <systemd/sd-daemon.h>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h> /* For accept */
#include <unistd.h> /* For read, errno, STDIN_FILENO */
#include <limits.h> /* For PIPE_BUF */
#include <string.h>
}

#include "measurement.h"
#include "analogdiscovery.h"
#include "gpio.h"
#include "default.h"
#include "tests.h"
#include "debug.h"


static const int BUF_SIZE = 256;
static char buf[BUF_SIZE];

int readRequest(int fd, std::string* result)
{
	ssize_t size = recv(fd, NULL, 0, MSG_PEEK|MSG_TRUNC);
	if (size <= 0)
		return size;

	size_t ret = 0;
	while (size > 0) {
		ret = read(fd, buf, size > BUF_SIZE ? BUF_SIZE : size);
		if (ret < 0)
			return ret;

		size -= ret;
		result->append(buf, ret);
	}

	return result->size();
}

//          <CMD> <Sub CMD> <Id>            <Param>
// Example: gpio    set      Power_Relais     1
//          gpio    get      Power_Relais
void parseRequest(int fd, SharedAnalogDiscoveryHandle device)
{
	std::string request;

	int ret = readRequest(fd, &request);
	if (ret < 0) {
		std::cerr << "Reading request failed: " << strerror(errno) << std::endl;
		return;
	}

	if (ret == 0) {
		/* Client connected then disconnected without sending message */
		return;
	}

	std::cout << "Request: " << request << std::endl;

	// Split up in tokens
	std::vector<std::string> tokens;
	std::istringstream iss(request);
	std::copy(std::istream_iterator<std::string>(iss),
			  std::istream_iterator<std::string>(),
			  std::back_inserter(tokens));

	if (tokens.size() == 3) {
		std::string cmd = tokens.at(0);
		std::string subCmd = tokens.at(1);
		std::string id = tokens.at(2);

		std::cout << "Tokens: (" << cmd << " | " << subCmd << " | " << id << ")" << std::endl;

		return;
	}

	std::cout << "SOME ERROR " << std::endl;
}



int main(int argc, char *argv[])
{
	try {

		int fd;

		if (sd_listen_fds(0) != 1) {
			std::cerr << "No or too many file descriptors received." << std::endl;
			exit(1);
		}

		fd = SD_LISTEN_FDS_START + 0;
		int ret = accept(fd, NULL, NULL);
		if (ret < 0) {
			std::cerr << "Accepting request failed: "  << strerror(errno) << std::endl;
			return 1;
		}

		auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
		parseRequest(ret, sharedDev);

	} catch (const AnalogDiscoveryException &e) {
		std::cerr << "AnalogDiscoveryException caught: " << e.what() << std::endl;
		std::cerr << where(e);
		exit(EXIT_FAILURE);
	} catch (const GPIOException &e) {
		std::cerr << "GPIOException caught: " << e.what() << std::endl;
		std::cerr << where(e);
		exit(EXIT_FAILURE);
	} catch (const std::exception &e) {
		std::cout << "std::exception caught: " << e.what() << std::endl;
		exit(EXIT_FAILURE);
	} catch (...) {
		std::cerr << "Unknown exception caught! Abort." << std::endl;
	}


	return 0;
}
