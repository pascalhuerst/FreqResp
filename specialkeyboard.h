#pragma once

class SpecialKeyboard
{
public:
	SpecialKeyboard();
	~SpecialKeyboard();
	char kbhit();
private:
	void nonblock(bool enable);

};

