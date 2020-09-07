/**
 * @file
 */

#include "MessageQueue.h"
#include "command/Command.h"
#include <algorithm>

namespace {
const double MessageDelay = 2.0;
}

void MessageQueue::message(const char *msg, ...) {
	va_list ap;
	const size_t bufSize = 4096;
	char buf[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(buf, bufSize, msg, ap);
	buf[sizeof(buf) - 1] = '\0';
	va_end(ap);

	_messageEventQueue.emplace_back(_timeSeconds + MessageDelay, buf);
	std::push_heap(_messageEventQueue.begin(), _messageEventQueue.end(), _messageEventQueueComp);
}

void MessageQueue::construct() {
	command::Command::registerCommand("addmessage", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		message("%s", args.front().c_str());
	});
}

bool MessageQueue::init() {
	return true;
}

void MessageQueue::shutdown() {
	_messageEventQueue.clear();
	command::Command::unregisterCommand("addmessage");
}

void MessageQueue::update(double deltaFrameSeconds) {
	_timeSeconds += deltaFrameSeconds;
	if (_messageEventQueue.empty()) {
		return;
	}
	// update queue and remove outdated messages
	for (;;) {
		const auto& msg = _messageEventQueue.front();
		const double remainingMillis = msg.ttlSeconds - _timeSeconds;
		if (remainingMillis > 0.0) {
			break;
		}
		std::pop_heap(_messageEventQueue.begin(), _messageEventQueue.end(), _messageEventQueueComp);
		_messageEventQueue.pop_back();
		if (_messageEventQueue.empty()) {
			break;
		}
	}
}
