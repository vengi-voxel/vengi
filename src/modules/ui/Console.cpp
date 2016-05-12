#include "Console.h"
#include "core/Command.h"
#include "core/Common.h"
#include "core/Color.h"

namespace ui {

Console::Console() {
	SDL_LogGetOutputFunction(&_logFunction, nullptr);
	SDL_LogSetOutputFunction(logConsole, this);

	_autoEnable = core::Var::get("ui_autoconsole", "false");
}

bool Console::init() {
	tb::TBFontDescription fd;
	fd.SetID(TBIDC("Segoe"));
	fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(20));
	tb::TBFontManager *fontMgr = tb::g_font_manager;

	if (fontMgr->HasFontFace(fd)) {
		_font = fontMgr->GetFontFace(fd);
	} else {
		_font = fontMgr->CreateFontFace(fd);
	}
	core_assert_msg(_font != nullptr, "Could not find the default font - make sure the ui is already configured");
	_font->RenderGlyphs(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNORSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~•·");

	core::Command::registerCommand("toggleconsole", [&] (const core::CmdArgs& args) { toggle(); });

	return true;
}

bool Console::onTextInput(const std::string& text) {
	if (_consoleActive)
		return false;

	_commandLine.append(text);

	return true;
}

void Console::logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	Console* console = (Console*)userdata;
	console->_logFunction(userdata, category, priority, message);
	// TODO: add color code for errors
	console->_messages.push_back(message);
	if (priority < SDL_LOG_PRIORITY_ERROR) {
		return;
	}
	if (!console->_consoleActive && console->_autoEnable->boolVal()) {
		console->toggle();
	}
}

bool Console::toggle() {
	_consoleActive ^= true;
	Log::info("toggle game console");
	if (_consoleActive) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
	return _consoleActive;
}

void Console::render(const tb::TBRect &rect) {
	static const tb::TBColor consoleFontColor(255, 255, 255);
	static const tb::TBColor consoleBgColor(127, 127, 127, 150);
	if (!_consoleActive) {
		return;
	}

	tb::g_renderer->DrawRectFill(rect, consoleBgColor);

	const int lineHeight = _font->GetFontDescription().GetSize();
	int maxY = _messages.size() * lineHeight;
	int y = std::min(rect.y + rect.h, maxY);
	for (MessagesIter i = _messages.rbegin(); i != _messages.rend(); ++i) {
		tb::TBStr str(i->c_str());
		if (y - lineHeight < 0) {
			break;
		}
		_font->DrawString(5, y, consoleFontColor, str);
		y -= lineHeight;
	}

	// TODO: draw input/commandline line
}

}
