/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Log.h"
#include "tb_core.h"
#include "tb_str.h"

namespace tb {

// == Platform interface ===================================================

/** TBSystem is porting interface for the underlaying OS. */
class TBSystem {
public:
	/** Get the system time in milliseconds since some undefined epoch. */
	static double getTimeMS();

	/** Called when the need to call TBMessageHandler::ProcessMessages has changed due to changes in the
		message queue. fire_time is the new time is needs to be called.
		It may be 0 which means that ProcessMessages should be called asap (but NOT from this call!)
		It may also be TB_NOT_SOON which means that ProcessMessages doesn't need to be called. */
	static void rescheduleTimer(double fireTime);

	/** Get how many milliseconds it should take after a touch down event should generate a long click
		event. */
	static int getLongClickDelayMS();

	/** Get how many pixels of dragging should start panning scrollable widgets. */
	static int getPanThreshold();

	/** Get how many pixels a typical line is: The length that should be scrolled when turning a mouse
		wheel one notch. */
	static int getPixelsPerLine();

	/** Get Dots Per Inch for the main screen. */
	static int getDPI();
};

/** TBClipboard is a porting interface for the clipboard. */
class TBClipboard {
public:
	/** Empty the contents of the clipboard. */
	static void empty();

	/** Return true if the clipboard currently contains text. */
	static bool hasText();

	/** Set the text of the clipboard in UTF-8 format. */
	static bool setText(const char *text);

	/** Get the text from the clipboard in UTF-8 format.
		Returns true on success. */
	static bool getText(TBStr &text);
};

/** TBFile is a porting interface for file access. */
class TBFile {
public:
	enum TBFileMode { MODE_READ };
	static TBFile *open(const char *filename, TBFileMode mode);

	virtual ~TBFile() {
	}
	virtual long size() = 0;
	virtual size_t read(void *buf, size_t elemSize, size_t count) = 0;
};

} // namespace tb
