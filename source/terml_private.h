#ifndef TERML_TERML_PRIVATE_H
#define TERML_TERML_PRIVATE_H

#include "terml.h"

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define ESC "\x1B"
#define CSI "\x1B["

#define ALT_BUF() CSI "?1049h"
#define REG_BUF() CSI "?1049l"

#define CLEAR_SCREEN() CSI "2J"

#define HIDE_CURSOR() CSI "?25l"
#define SHOW_CURSOR() CSI "?25h"

#define REPORT_CUSROR_POSITION() CSI "6n"
#define CURSOR_POSITION_FORMAT() "%*1s[%u;%u"

#define CUP(x, y) CSI STRINGIFY(y) ";" STRINGIFY(x) "H"

#define FG(r, g, b) CSI "38;2;" STRINGIFY(r) ";" STRINGIFY(g) ";" STRINGIFY(b) "m"
#define BG(r, g, b) CSI "48;2;" STRINGIFY(r) ";" STRINGIFY(g) ";" STRINGIFY(b) "m"

class terml
{
public:
	terml();
	virtual ~terml();

	terml(const terml&) = delete;
	terml(terml&&) = delete;

	terml& operator=(const terml&) = delete;
	terml& operator=(terml&&) = delete;

	char get(unsigned int x, unsigned int y, int* fg, int* bg) const;
	void set(unsigned int x, unsigned int y, char c, int fg, int bg);
	void flush() const;

	void set_main_callback(terml_main_callback);
	void set_quit_callback(terml_quit_callback);
	void set_key_callback(terml_key_callback);
	void set_resize_callback(terml_resize_callback);

	void mainloop();
	void stop();

	unsigned int get_width() const;
	unsigned int get_height() const;

	void setup_buffer();

	virtual void set_console_settings() = 0;
	virtual void reset_console_settings() = 0;

protected:
	virtual void read_stdin(char* buffer, unsigned int buffer_size) = 0;
	virtual unsigned long long timer() = 0;
	virtual unsigned long long timer_frequency() = 0;
	virtual void process_events() = 0;

	void key_event(char code) const;

private:
	char* buffer;
	unsigned int width;
	unsigned int height;
	terml_main_callback main;
	terml_quit_callback quit;
	terml_key_callback key;
	terml_resize_callback resize;
	bool should_quit;
	bool really_should_quit;

	static const unsigned int CELL_SIZE = sizeof(FG(255, 255, 255) BG(255, 255, 255) " ") - 1;
};

#endif//TERML_TERML_PRIVATE_H