/**
 * @file
 */

#include "MessageQueue.h"
#include "core/command/Command.h"

namespace {
const uint64_t MessageDelay = 6000;
}

void MessageQueue::message(const char *msg, ...) {
	va_list ap;
	constexpr std::size_t bufSize = 4096;
	char buf[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(buf, bufSize, msg, ap);
	buf[sizeof(buf) - 1] = '\0';
	va_end(ap);

	_messageEventQueue.emplace_back(_time + MessageDelay, buf);
	std::push_heap(_messageEventQueue.begin(), _messageEventQueue.end(), _messageEventQueueComp);
}

void MessageQueue::construct() {
	core::Command::registerCommand("addmessage", [&] (const core::CmdArgs& args) {
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
	core::Command::unregisterCommand("addmessage");
}

void MessageQueue::update(uint64_t dt) {
	_time += dt;
	if (_messageEventQueue.empty()) {
		return;
	}
	// update queue and remove outdated messages
	for (;;) {
		const auto& msg = _messageEventQueue.front();
		const int64_t remainingMillis = msg.ttl - _time;
		if (remainingMillis > 0) {
			break;
		}
		std::pop_heap(_messageEventQueue.begin(), _messageEventQueue.end(), _messageEventQueueComp);
		_messageEventQueue.pop_back();
		if (_messageEventQueue.empty()) {
			break;
		}
	}
}
