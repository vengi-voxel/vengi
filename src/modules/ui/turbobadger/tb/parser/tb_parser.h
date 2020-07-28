/**
 * @file
 */

#pragma once

#include "tb_str.h"
#include "tb_tempbuffer.h"
#include "tb_value.h"

namespace tb {

/** Unescape backslash codes. This is done in place using the string both as source
	and destination. */
void unescapeString(char *str);

/** Check if buf is pointing at an end quote. It may need to iterate
	buf backwards toward buf_start to check if any preceding backslashes
	make it a escaped quote (which should not be the end quote) */
bool isEndQuote(const char *buf_start, const char *buf, const char quote_type);

class TBParserTarget {
public:
	virtual ~TBParserTarget() {
	}
	virtual void onError(int line_nr, const char *error) = 0;
	virtual void onComment(int line_nr, const char *comment) = 0;
	virtual void onToken(int line_nr, const char *name, TBValue &value) = 0;
	virtual void enter() = 0;
	virtual void leave() = 0;
};

class TBParserStream {
public:
	virtual ~TBParserStream() {
	}
	virtual int getMoreData(char *buf, int buf_len) = 0;
};

class TBParser {
public:
	enum STATUS { STATUS_OK, STATUS_OUT_OF_MEMORY, STATUS_PARSE_ERROR };
	TBParser() {
	}
	STATUS read(TBParserStream *stream, TBParserTarget *target);

private:
	int current_indent = 0;
	int current_line_nr = 0;
	core::String multi_line_token;
	TBTempBuffer multi_line_value;
	int multi_line_sub_level = 0;
	bool pending_multiline = false;
	void onLine(char *line, TBParserTarget *target);
	void onCompactLine(char *line, TBParserTarget *target);
	void onMultiline(char *line, TBParserTarget *target);
	void consumeValue(TBValue &dst_value, char *&line);
};

} // namespace tb
