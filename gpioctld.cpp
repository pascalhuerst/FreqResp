#include <iostream>
#include <thread>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h> /* For accept */
#include <unistd.h> /* For read, errno, STDIN_FILENO */
#include <limits.h> /* For PIPE_BUF */
}

#include "measurement.h"
#include "analogdiscovery.h"
#include "gpio.h"
#include "default.h"
#include "tests.h"
#include "debug.h"


using namespace std;

/* Read a SEQPACKET message from the request socket into the result string.
 *
 * Since SEQPACKET sockets require the whole message be read in one syscall
 * and we don't know ahead of time how large the result will be,
 * we use MSG_PEEK|MSG_TRUNC to find out how large the message is,
 * so we can ensure we have a buffer large enough, then read into it.
 */
int read_request(int request_fd, string &result)
{
	ssize_t ret = TEMP_FAILURE_RETRY(recv(request_fd, NULL, 0, MSG_PEEK|MSG_TRUNC));
	if (ret <= 0) {
		return ret;
	}

	ssize_t msglen = ret;
	result.resize(msglen);

	ret = TEMP_FAILURE_RETRY(read(request_fd, &result[0], msglen));

	return ret;
}

/* Handle requests of the form "<GPIO NAME> <on|off>"
 *
 * The client writes one message before of the name of the GPIO
 * and whether to set the state to "on" or "off".
 *
 * The client will wait for a response or a disconnect without response.
 *
 * Responses either start "Invalid request: " for incorrect input,
 * or "Success: " for a successful call.
 */
void handle_request(int request_fd, list<shared_ptr<GPIO> > const &gpios)
{
	string request;
	int ret = read_request(request_fd, request);
	if (ret < 0) {
		cerr << "Reading request failed" << endl;
		return;
	}

	if (ret == 0) {
		/* Client connected then disconnected without sending message */
		return;
	}

	size_t word_sep = request.rfind(' ');
	if (word_sep == string::npos) {
		/* No space found, malformed */
		char response[] = "Invalid request: missing word separator";
		TEMP_FAILURE_RETRY(write(request_fd, response, sizeof response));
		return;
	}

	string gpio_name = request.substr(0, word_sep);
	string value = request.substr(word_sep + 1, string::npos);

	SharedGPIOHandle gpio;
	try {
		gpio = getGPIOForName(gpios, gpio_name);
	} catch (GPIOException e) {
		/* No gpio by that name found */
		string response("Invalid request: found no GPIO called: ");
		response += gpio_name;
		TEMP_FAILURE_RETRY(write(request_fd, response.c_str(), response.length()));
		return;
	}

	if (value == "on") {
		gpio->setValue(true);
	} else if (value == "off") {
		gpio->setValue(false);
	} else {
		/* Invalid value to set */
		string response("Invalid request: invalid value: ");
		response += value;
		TEMP_FAILURE_RETRY(write(request_fd, response.c_str(), response.length()));
		return;
	}

	string response("Success: set ");
	response += gpio_name;
	response += " to ";
	response += value;
	TEMP_FAILURE_RETRY(write(request_fd, response.c_str(), response.length()));
}

int main(int argc, char *argv[])
{
	auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
	auto gpios = loadDefaultGPIOMapping(sharedDev);

	for (;;) {
		/* Listening UNIX socket fd is passed as stdin */
		int ret = TEMP_FAILURE_RETRY(accept(STDIN_FILENO, NULL, NULL));
		if (ret < 0) {
			cerr << "Accepting request failed" << endl;
			return 1;
		}
		int request_fd = ret;

		handle_request(request_fd, gpios);

		TEMP_FAILURE_RETRY(close(request_fd));
	}

	return 0;
}
