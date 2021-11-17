#pragma once

#include <ostream>
#include <utility>

enum class color
{
	unchanged,
	red,
	green,
	white,
	yellow,
	blue,
	magenta,
	cyan,
	lightred,
	lightgreen,
	lightyellow,
	lightblue,
	lightmagenta,
	lightcyan,
	black
};

class color_provider_interface
{
public:
	virtual ~color_provider_interface() = default;

	virtual color set_foreground(std::ostream& stream, color value) = 0;
	virtual color set_background(std::ostream& stream, color value) = 0;
	virtual void reset(std::ostream& stream) = 0;
};

class empty_color_provider : public color_provider_interface
{
	virtual color set_foreground(std::ostream&, color) override
	{
		return color::unchanged;
	}

	virtual color set_background(std::ostream&, color) override
	{
		return color::unchanged;
	}

	virtual void reset(std::ostream&) override
	{
	}
};

class escape_sequence_color_provider : public color_provider_interface
{
	virtual color set_foreground(std::ostream& stream, color value) override
	{
		using enum color;
		switch (value)
		{
		case red: stream << "\033[31m"; break;
		case green: stream << "\033[32m"; break;
		case white: stream << "\033[97m"; break;
		case yellow: stream << "\033[33m"; break;
		case blue: stream << "\033[34m"; break;
		case magenta: stream << "\033[35m"; break;
		case cyan: stream << "\033[36m"; break;
		case black: stream << "\033[30m"; break;
		case lightred: stream << "\033[91m"; break;
		case lightgreen: stream << "\033[92m"; break;
		case lightyellow: stream << "\033[93m"; break;
		case lightblue: stream << "\033[94m"; break;
		case lightmagenta: stream << "\033[95m"; break;
		case lightcyan: stream << "\033[96m"; break;
		default: break;
		}

		auto result = prev_foreground_;
		prev_foreground_ = value;
		return result;
	}

	virtual color set_background(std::ostream& stream, color value) override
	{
		using enum color;
		switch (value)
		{
		case red: stream << "\033[41m"; break;
		case green: stream << "\033[42m"; break;
		case white: stream << "\033[107m"; break;
		case yellow: stream << "\033[43m"; break;
		case blue: stream << "\033[44m"; break;
		case magenta: stream << "\033[45m"; break;
		case cyan: stream << "\033[46m"; break;
		case black: stream << "\033[40m"; break;
		case lightred: stream << "\033[101m"; break;
		case lightgreen: stream << "\033[102m"; break;
		case lightyellow: stream << "\033[103m"; break;
		case lightblue: stream << "\033[104m"; break;
		case lightmagenta: stream << "\033[105m"; break;
		case lightcyan: stream << "\033[106m"; break;
		default: break;
		}

		auto result = prev_background_;
		prev_background_ = value;
		return result;
	}

	virtual void reset(std::ostream& stream) override
	{
		stream << "\033[0m";
	}

private:
	color prev_foreground_ = color::unchanged;
	color prev_background_ = color::unchanged;
};

struct color_changer
{
public:
	color_changer(std::ostream& stream,
		color_provider_interface& color_provider,
		color foreground, color background)
		: stream_(stream)
		, color_provider_(color_provider)
	{
		if (foreground != color::unchanged)
			color_provider_.set_foreground(stream_, foreground);
		if (background != color::unchanged)
			color_provider_.set_background(stream_, background);
	}

	~color_changer()
	{
		try
		{
			color_provider_.reset(stream_);
		}
		catch (...)
		{
		}
	}

	color_changer(const color_changer&) = delete;
	color_changer& operator=(const color_changer&) = delete;
	color_changer(color_changer&&) = delete;
	color_changer& operator=(color_changer&&) = delete;

private:
	std::ostream& stream_;
	color_provider_interface& color_provider_;
};
