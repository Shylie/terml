#include "terml_linux.h"

#ifndef _WIN32

#include <time.h>
#include <errno.h>
#include <cstdio>
#include <poll.h>

void terml_linux::set_console_settings_impl()
{
	tcgetattr(STDIN_FILENO, &old_input_settings);
	tcgetattr(STDOUT_FILENO, &old_output_settings);

	termios t = old_input_settings;
	t.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	printf(SELECT_UTF8());
	fflush(stdout);
}

void terml_linux::reset_console_settings_impl()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_input_settings);
	tcsetattr(STDOUT_FILENO, TCSANOW, &old_output_settings);
}

void terml_linux::read_stdin(char* buffer, unsigned int buffer_size)
{
	for (int i = 0; i < buffer_size;)
	{
		char ch = getchar();
		if (i == 0 && ch != '\x1B')
		{
			key_event(ch);
		}
		else
		{
			buffer[i++] = ch;
		}

		if (ch == 'R') { return; }
	}
}

unsigned long long terml_linux::timer()
{
	timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);

	return timer_frequency() * spec.tv_sec + spec.tv_nsec;
}

unsigned long long terml_linux::timer_frequency()
{
	return 1000000000; // nanoseconds / second
}

void terml_linux::process_events()
{
	constexpr int BUF_SIZE = 128;
	char buf[BUF_SIZE];
	int num_read;

	pollfd pfd;
	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	pfd.revents = 0;

	poll(&pfd, 1, 0);

	while (pfd.revents & POLLIN)
	{
		num_read = read(STDIN_FILENO, buf, BUF_SIZE);
		for (int i = 0; i < num_read; i++)
		{
			key_event(buf[i]);
		}

		poll(&pfd, 1, 0);
	}

	setup_buffer();
}

#endif