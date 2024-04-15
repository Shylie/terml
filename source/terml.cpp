#include "terml_private.h"

#ifdef _WIN32
#include "terml_windows.h"
#else
#include "terml_linux.h"
#endif

#include <cstdio>
#include <cstring>

namespace
{
	constexpr int BYTE_ONE_MASK   = 0b000000000000000000111111;
	constexpr int BYTE_TWO_MASK   = 0b000000000000111111000000;
	constexpr int BYTE_THREE_MASK = 0b000000111111000000000000;
	constexpr int BYTE_FOUR_MASK  = 0b000111000000000000000000;

	constexpr int BYTE_ONE_MASK_OFFSET   = 0;
	constexpr int BYTE_TWO_MASK_OFFSET   = BYTE_ONE_MASK_OFFSET   + 2;
	constexpr int BYTE_THREE_MASK_OFFSET = BYTE_TWO_MASK_OFFSET   + 2;
	constexpr int BYTE_FOUR_MASK_OFFSET  = BYTE_THREE_MASK_OFFSET + 2;

	constexpr int ONE_BYTE_FILL   =                         0b10000000;
	constexpr int TWO_BYTE_FILL   =                 0b1100000010000000;
	constexpr int THREE_BYTE_FILL =         0b111000001000000010000000;
	constexpr int FOUR_BYTE_FILL  = 0b11110000100000001000000010000000;

	void print_cell_impl(tcell cell)
	{
		// one-byte codepoints
		if (cell.codepoint < 0x80)
		{
			const char str[] = { cell.codepoint, '\0' };
			printf("%s", str);
		}
		// two-byte codepoints
		else if (cell.codepoint < 0x800)
		{
			const int byte_one = (cell.codepoint & BYTE_ONE_MASK) << BYTE_ONE_MASK_OFFSET;
			const int byte_two = (cell.codepoint & BYTE_TWO_MASK) << BYTE_TWO_MASK_OFFSET;
			const int utf8 = TWO_BYTE_FILL | byte_one | byte_two;
			
			const char str[] = { (utf8 & 0xFF00) >> 8, utf8 & 0xFF, '\0' };
			printf("%s", str);
		}
		// three-byte codepoints
		else if (cell.codepoint < 0x10000)
		{
			const int byte_one   = (cell.codepoint & BYTE_ONE_MASK)   << BYTE_ONE_MASK_OFFSET;
			const int byte_two   = (cell.codepoint & BYTE_TWO_MASK)   << BYTE_TWO_MASK_OFFSET;
			const int byte_three = (cell.codepoint & BYTE_THREE_MASK) << BYTE_THREE_MASK_OFFSET;
			const int utf8 = THREE_BYTE_FILL | byte_one | byte_two | byte_three;

			const char str[] = { (utf8 & 0xFF0000) >> 16, (utf8 & 0xFF00) >> 8, utf8 & 0xFF, '\0' };
			printf("%s", str);
		}
		// four-byte codepoints
		else if (cell.codepoint < 0x110000)
		{
			const int byte_one   = (cell.codepoint & BYTE_ONE_MASK)   << BYTE_ONE_MASK_OFFSET;
			const int byte_two   = (cell.codepoint & BYTE_TWO_MASK)   << BYTE_TWO_MASK_OFFSET;
			const int byte_three = (cell.codepoint & BYTE_THREE_MASK) << BYTE_THREE_MASK_OFFSET;
			const int byte_four  = (cell.codepoint & BYTE_FOUR_MASK)  << BYTE_FOUR_MASK_OFFSET;
			const int utf8 = FOUR_BYTE_FILL | byte_one | byte_two | byte_three | byte_four;

			const char str[] = { (utf8 & 0xFF000000) >> 24, (utf8 & 0xFF0000) >> 16, (utf8 & 0xFF00) >> 8, utf8 & 0xFF, '\0' };
			printf("%s", str);
		}
	}

	void print_cell(tcell cell, const tcell* last = nullptr)
	{
		if (!last || cell.foreground != last->foreground)
		{
			printf(FG(%d, %d, %d), (cell.foreground & 0xFF0000) >> 16, (cell.foreground & 0xFF00) >> 8, cell.foreground & 0xFF);
		}
		if (!last || cell.background != last->background)
		{
			printf(BG(%d, %d, %d), (cell.background & 0xFF0000) >> 16, (cell.background & 0xFF00) >> 8, cell.background & 0xFF);
		}

		print_cell_impl(cell);
	}

	bool operator==(const tcell& a, const tcell& b)
	{
		return
			a.background == b.background &&
			a.foreground == b.foreground &&
			a.codepoint  == b.codepoint;
	}

	bool operator!=(const tcell& a, const tcell& b)
	{
		return
			a.background != b.background ||
			a.foreground != b.foreground ||
			a.codepoint  != b.codepoint;
	}
}

terml::terml() :
	cells(nullptr),
	width(0),
	height(0),
	main(nullptr),
	quit(nullptr),
	key(nullptr),
	resize(nullptr),
	should_quit(false),
	really_should_quit(false)
{
}

terml::~terml()
{
	if (cells) { delete[] cells; }
}

const tcell& terml::get(unsigned int x, unsigned int y) const
{
	return cells[x + y * width].cell;
}

void terml::set(unsigned int x, unsigned int y, tcell cell)
{
	cells[x + y * width].dirty = (cell != cells[x + y * width].cell);
	cells[x + y * width].cell = cell;
}

void terml::flush() const
{
	const tcell* last = nullptr;
	if (cells[0].dirty)
	{
		printf(CUP(1, 1));
		print_cell(cells[0].cell);
		cells[0].dirty = false;

		last = &cells[0].cell;
	}
	for (int i = 1; i < width * height; i++)
	{
		const unsigned int x = i % width;
		const unsigned int y = i / width;

		if (cells[i].dirty)
		{
			printf(CUP(%d, %d), y + 1, x + 1);
			print_cell(cells[i].cell, last);
			cells[i].dirty = false;
			
			last = &cells[i].cell;
		}
	}

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

	const unsigned long long wait_time = timer_frequency() / 60;
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

	const unsigned int new_width = width;
	const unsigned int new_height = height;
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

		if (cells) { delete[] cells; }
		cells = new tcelld[width * height];
		memset(cells, 0, sizeof(tcelld) * width * height);

		if (resize)
		{
			resize(old_width, old_height);
		}
	}
}

void terml::set_console_settings()
{
	setvbuf(stdout, nullptr, _IOFBF, BUFSIZ * BUFSIZ);
	printf(ALT_BUF() HIDE_CURSOR());
	fflush(stdout);

	set_console_settings_impl();
}

void terml::reset_console_settings()
{
	setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);
	printf(REG_BUF() SHOW_CURSOR());
	fflush(stdout);

	reset_console_settings_impl();
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
			else
			{
				throw "terml already initialized.";
			}

			TERML_G->set_console_settings();
			TERML_G->setup_buffer();

			return 0;
		}
		catch (const char* c)
		{
			LAST_ERROR = c;
			return 1;
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

			return 0;
		}
		catch (const char* c)
		{
			LAST_ERROR = c;
			return 1;
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

	int terml_get(unsigned int x, unsigned int y, tcell* cell)
	{
		if (x >= TERML_G->get_width() || y >= TERML_G->get_height())
		{
			LAST_ERROR = "Coordinates out of bounds.";
			return 1;
		}
		else if (!cell)
		{
			LAST_ERROR = "Null output pointer.";
			return 1;
		}
		else
		{
			*cell = TERML_G->get(x, y);
			return 0;
		}
	}

	int terml_set(unsigned int x, unsigned int y, tcell cell)
	{
		if (x >= TERML_G->get_width() || y >= TERML_G->get_height())
		{
			LAST_ERROR = "Coordinates out of bounds.";
			return 1;
		}
		else
		{
			TERML_G->set(x, y, cell);
			return 0;
		}
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