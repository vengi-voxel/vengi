/**
 * @file
 */

#include "I18N.h"
#include "App.h"

#ifndef IMGUI_ENABLE_TEST_ENGINE
namespace app {

const char *translate(const char *msgid) {
	return App::getInstance()->translate(msgid);
}

const char *translateCtxt(const char *msgctxt, const char *msgid) {
	return App::getInstance()->translateCtxt(msgctxt, msgid);
}

} // namespace app
#endif
