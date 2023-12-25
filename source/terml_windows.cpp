#include "terml_windows.h"

#ifdef _WIN32

void terml_windows::set_console_settings()
{
	handle_stdin = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(handle_stdin, &previous_input_mode);
	DWORD new_mode = previous_input_mode;
	new_mode &= ~ENABLE_ECHO_INPUT;
	new_mode &= ~ENABLE_LINE_INPUT;
	new_mode |= ENABLE_WINDOW_INPUT;
	new_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	if (!SetConsoleMode(handle_stdin, new_mode))
	{
		throw "Failed to set stdin mode";
	}

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(handle, &previous_output_mode);
	new_mode = previous_output_mode;
	new_mode |= ENABLE_PROCESSED_OUTPUT;
	new_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(handle, new_mode))
	{
		throw "Failed to set stdout mode";
	}
}

void terml_windows::reset_console_settings()
{
	if (!SetConsoleMode(handle_stdin, previous_input_mode))
	{
		throw "Failed to reset stdin mode";
	}
	if (!SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), previous_output_mode))
	{
		throw "Failed to reset stdout mode";
	}
}

void terml_windows::read_stdin(char* buffer, unsigned int buffer_size)
{
	unsigned long unused;
	ReadConsole(handle_stdin, buffer, buffer_size, &unused, nullptr);
}

unsigned long long terml_windows::timer()
{
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	return time.QuadPart;
}

unsigned long long terml_windows::timer_frequency()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

void terml_windows::process_events()
{
	DWORD num_events_available = 0;
	GetNumberOfConsoleInputEvents(handle_stdin, &num_events_available);

	DWORD num_events_read = 0;
	INPUT_RECORD input_record_buffer[128];
	while (num_events_available > 0)
	{
		ReadConsoleInput(handle_stdin, input_record_buffer, sizeof(input_record_buffer) / sizeof(*input_record_buffer), &num_events_read);

		for (int i = 0; i < num_events_read; i++)
		{
			switch (input_record_buffer[i].EventType)
			{
			case KEY_EVENT:
			{
				KEY_EVENT_RECORD* record = &input_record_buffer[i].Event.KeyEvent;
				if (record->bKeyDown)
				{
					for (int repeat = 0; repeat < record->wRepeatCount; repeat++)
					{
						key_event(record->uChar.AsciiChar);
					}
				}
				break;
			}

			case WINDOW_BUFFER_SIZE_EVENT:
				setup_buffer();
				break;
			}
		}

		num_events_available -= num_events_read;
	}
}

#endif