#ifndef TERML_TERML_LINUX_H
#define TERML_TERML_LINUX_H

#include "terml_private.h"

#ifndef _WIN32

#include <termios.h>
#include <unistd.h>

class terml_linux : public terml
{
protected:
	virtual void set_console_settings() override;
	virtual void reset_console_settings() override;
	virtual void read_stdin(char* buffer, unsigned int buffer_size) override;
	virtual unsigned long long timer() override;
	virtual unsigned long long timer_frequency() override;
	virtual void process_events() override;

private:
	termios old_input_settings;
	termios old_output_settings;
};

#endif//_WIN32

#endif//TERML_TERML_LINUX_H