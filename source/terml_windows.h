#ifndef TERML_TERML_WINDOWS_H
#define TERML_TERML_WINDOWS_H

#include "terml_private.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class terml_windows : public terml
{
protected:
	virtual void set_console_settings() override;
	virtual void reset_console_settings() override;
	virtual void read_stdin(char* buffer, unsigned int buffer_size) override;
	virtual unsigned long long timer() override;
	virtual unsigned long long timer_frequency() override;
	virtual void process_events() override;

private:
	DWORD previous_input_mode;
	DWORD previous_output_mode;
	HANDLE handle_stdin;
};

#endif//_WIN32

#endif//TERML_TERML_WINDOWS_H