#pragma once

#include "TurboBadger.h"
#include "core/Common.h"

namespace ui {

class Font {
protected:
	int _maxLength = 512;
	mutable tb::TBFontFace *_font = nullptr;
	glm::vec4 _color;
	glm::ivec2 _pos;

public:
	bool init(const char *font = "Segoe", int size = 20);
	bool init(tb::TBFontFace *font);

	inline Font& color(const glm::vec4& color) {
		_color = color;
		return *this;
	}

	inline Font& pos(const glm::ivec2& pos) {
		_pos = pos;
		return *this;
	}

	inline Font& pos(int x, int y) {
		return pos(glm::ivec2(x, y));
	}

	inline Font& max(int maxLength) {
		_maxLength = maxLength;
		core_assert_msg(_maxLength <= 4096, "exceeded max length");
		core_assert_msg(_maxLength > 0, "max length should be bigger than 0");
		return *this;
	}

	inline int getWidth(const std::string& str, int length = 4096) const {
		return getWidth(str.c_str(), length);
	}

	inline int getWidth(const char* str, int length = 4096) const {
		const int l = _font->GetStringWidth(str, length);
		return l;
	}

	inline int getSize() const {
		return _font->GetFontDescription().GetSize();
	}

	Font& draw(const char *format, ...) __attribute__((format(printf, 2, 3)));

	inline Font& draw(const std::string& str) {
		return draw("%s", str.c_str());
	}
};

}
