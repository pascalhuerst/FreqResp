#include "specialkeyboard.h"
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

SpecialKeyboard::SpecialKeyboard()
{
	nonblock(true);
}

SpecialKeyboard::~SpecialKeyboard()
{
	nonblock(false);
}

char SpecialKeyboard::kbhit()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

	if (FD_ISSET(STDIN_FILENO, &fds)) {
		char ret;
		read(STDIN_FILENO, &ret, 1);
		return ret;
	}
	return 0;
}

void SpecialKeyboard::nonblock(bool enable)
{
	struct termios ttystate;

	//get the terminal state
	tcgetattr(STDIN_FILENO, &ttystate);

	if (enable) {
		//turn off canonical mode
		ttystate.c_lflag &= ~ICANON;
		//minimum of number input read.
		ttystate.c_cc[VMIN] = 1;
		//no echo
		ttystate.c_lflag &= ~ECHO;
	} else {
		ttystate.c_lflag |= ECHO;
		ttystate.c_lflag |= ICANON;
	}
	//set the terminal attributes.
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}
