#include "terml_private.h"

#ifdef _WIN32
#include "terml_windows.h"
#else
#include "terml_linux.h"
#endif

#include <cstdio>
#include <cstring>

terml::terml() :
	main(nullptr),
	quit(nullptr),
	key(nullptr),
	resize(nullptr),
	buffer(nullptr),
	width(0),
	height(0)
{
	printf(ALT_BUF() HIDE_CURSOR());
	fflush(stdout);
}

terml::~terml()
{
	printf(REG_BUF() SHOW_CURSOR());
	fflush(stdout);

	delete[] buffer;
}

char terml::get(unsigned int x, unsigned int y, int* fg, int* bg) const
{
	const unsigned int offset = x + y * width;

	if (fg)
	{
		const unsigned char r =
			(buffer[offset * CELL_SIZE + 9] - '0') +
			(buffer[offset * CELL_SIZE + 8] - '0') * 10 +
			(buffer[offset * CELL_SIZE + 7] - '0') * 100;

		const unsigned char g =
			(buffer[offset * CELL_SIZE + 13] - '0') +
			(buffer[offset * CELL_SIZE + 12] - '0') * 10 +
			(buffer[offset * CELL_SIZE + 11] - '0') * 100;

		const unsigned char b =
			(buffer[offset * CELL_SIZE + 17] - '0') +
			(buffer[offset * CELL_SIZE + 16] - '0') * 10 +
			(buffer[offset * CELL_SIZE + 15] - '0') * 100;

		*fg = (r << 16) | (g << 8) | b;
	}

	if (bg)
	{
		const unsigned char r =
			(buffer[offset * CELL_SIZE + 28] - '0') +
			(buffer[offset * CELL_SIZE + 27] - '0') * 10 +
			(buffer[offset * CELL_SIZE + 26] - '0') * 100;

		const unsigned char g =
			(buffer[offset * CELL_SIZE + 32] - '0') +
			(buffer[offset * CELL_SIZE + 31] - '0') * 10 +
			(buffer[offset * CELL_SIZE + 30] - '0') * 100;

		const unsigned char b =
			(buffer[offset * CELL_SIZE + 36] - '0') +
			(buffer[offset * CELL_SIZE + 35] - '0') * 10 +
			(buffer[offset * CELL_SIZE + 34] - '0') * 100;

		*bg = (r << 16) | (g << 8) | b;
	}

	return buffer[offset * CELL_SIZE + CELL_SIZE - 1];
}

void terml::set(unsigned int x, unsigned int y, char c, int fg, int bg)
{
	const unsigned int offset = x + y * width;

	unsigned char fgr = (fg & 0xFF0000) >> 16;
	unsigned char fgg = (fg & 0x00FF00) >> 8;
	unsigned char fgb = (fg & 0x0000FF);

	unsigned char bgr = (bg & 0xFF0000) >> 16;
	unsigned char bgg = (bg & 0x00FF00) >> 8;
	unsigned char bgb = (bg & 0x0000FF);

	for (int i = 0; i < 3; i++)
	{
		buffer[offset * CELL_SIZE + (9 - i)] = (fgr % 10) + '0';
		fgr /= 10;

		buffer[offset * CELL_SIZE + (13 - i)] = (fgg % 10) + '0';
		fgg /= 10;

		buffer[offset * CELL_SIZE + (17 - i)] = (fgb % 10) + '0';
		fgb /= 10;

		buffer[offset * CELL_SIZE + (28 - i)] = (bgr % 10) + '0';
		bgr /= 10;

		buffer[offset * CELL_SIZE + (32 - i)] = (bgg % 10) + '0';
		bgg /= 10;

		buffer[offset * CELL_SIZE + (36 - i)] = (bgb % 10) + '0';
		bgb /= 10;
	}

	buffer[offset * CELL_SIZE + CELL_SIZE - 1] = c;
}

void terml::flush() const
{
	printf(CUP(1, 1));
	fflush(stdout);

	printf("%s", buffer);
	fflush(stdout);
}

void terml::set_main_callback(terml_main_callback callback)
{
	main = callback;
}

void terml::set_quit_callback(terml_quit_callback callback)
{
	quit = callback;
}

void terml::set_key_callback(terml_key_callback callback)
{
	key = callback;
}

void terml::set_resize_callback(terml_resize_callback callback)
{
	resize = callback;
}

void terml::key_event(char code) const
{
	if (key)
	{
		key(code);
	}
}

void terml::mainloop()
{
	should_quit = false;
	really_should_quit = false;

	const unsigned long long wait_time = timer_frequency() / 1000;
	unsigned long long last_time = timer();

	while (!really_should_quit)
	{
		unsigned long long current_time = timer();

		while (current_time >= last_time + wait_time)
		{
			if (main)
			{
				main();
			}

			process_events();

			last_time = current_time;
		}

		if (should_quit)
		{
			if (!quit || quit())
			{
				really_should_quit = true;
			}
			else
			{
				should_quit = false;
			}
		}
	}
}

void terml::stop()
{
	should_quit = true;
}

unsigned int terml::get_width() const
{
	return width;
}

unsigned int terml::get_height() const
{
	return height;
}

void terml::setup_buffer()
{
	printf(CUP(999, 999) REPORT_CUSROR_POSITION());
	fflush(stdout);

	constexpr const unsigned int STDIN_BUFFER_SIZE = 16;
	char stdin_buffer[STDIN_BUFFER_SIZE + 1];
	read_stdin(stdin_buffer, STDIN_BUFFER_SIZE);

	stdin_buffer[STDIN_BUFFER_SIZE] = '\0';

	unsigned int new_width = width;
	unsigned int new_height = height;
	const int scanned = sscanf(stdin_buffer, CURSOR_POSITION_FORMAT(), &new_height, &new_width);
	if (scanned != 2)
	{
		throw "Failed to determine screen size.";
	}

	if (width != new_width || height != new_height)
	{
		const unsigned int old_width = width;
		const unsigned int old_height = height;

		width = new_width;
		height = new_height;

		if (buffer) { delete[] buffer; }
		buffer = new char[CELL_SIZE * width * height + 1];

		constexpr char BLANK_CELL[] = FG(255, 255, 255) CSI "48;2;000;000;000m" " ";

		for (int i = 0; i < width * height; i++)
		{
			memcpy(buffer + i * CELL_SIZE, BLANK_CELL, CELL_SIZE);
		}

		buffer[CELL_SIZE * width * height] = '\0';

		if (resize)
		{
			resize(old_width, old_height, new_width, new_height);
		}
	}
}

static terml* TERML_G;
static const char* LAST_ERROR = nullptr;

extern "C"
{
	int terml_init()
	{
		try
		{
			if (!TERML_G)
			{
#ifdef _WIN32
				TERML_G = new terml_windows;
#else
				TERML_G = new terml_linux;
#endif
			}

			TERML_G->set_console_settings();
			TERML_G->setup_buffer();

			return 1;
		}
		catch (const char* c)
		{
			LAST_ERROR = c;
			return 0;
		}
	}

	int terml_deinit()
	{
		try
		{
			if (TERML_G)
			{
				TERML_G->reset_console_settings();
				delete TERML_G;
			}

			return 1;
		}
		catch (const char* c)
		{
			LAST_ERROR = c;
			return 0;
		}
	}

	unsigned int terml_get_width()
	{
		return TERML_G->get_width();
	}

	unsigned int terml_get_height()
	{
		return TERML_G->get_height();
	}

	char terml_get(unsigned int x, unsigned int y, int* fg, int* bg)
	{
		return TERML_G->get(x, y, fg, bg);
	}

	void terml_set(unsigned int x, unsigned int y, char c, int fg, int bg)
	{
		TERML_G->set(x, y, c, fg, bg);
	}

	void terml_flush()
	{
		TERML_G->flush();
	}

	void terml_set_main_callback(terml_main_callback callback)
	{
		TERML_G->set_main_callback(callback);
	}

	void terml_set_quit_callback(terml_quit_callback callback)
	{
		TERML_G->set_quit_callback(callback);
	}

	void terml_set_key_callback(terml_key_callback callback)
	{
		TERML_G->set_key_callback(callback);
	}

	void terml_set_resize_callback(terml_resize_callback callback)
	{
		TERML_G->set_resize_callback(callback);
	}

	void terml_start()
	{
		try
		{
			TERML_G->mainloop();
		}
		catch (const char* c)
		{
			LAST_ERROR = c;
		}
	}

	void terml_stop()
	{
		TERML_G->stop();
	}

	const char* terml_get_error()
	{
		const char* err = LAST_ERROR;
		LAST_ERROR = nullptr;
		return err;
	}
}