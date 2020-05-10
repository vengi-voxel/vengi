/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <stdint.h>
#include <vector>

/**
 * @brief Class that implements messages with lifetime. The messages are removed once they got old enough.
 *
 * This can e.g. be used to display hud messages
 */
class MessageQueue : public core::IComponent {
private:
	struct MessageEvent {
		MessageEvent(double _ttlSeconds, const core::String& _msg) :
				ttlSeconds(_ttlSeconds), msg(_msg) {
		}
		double ttlSeconds;
		core::String msg;
	};

	struct MessageEventComparator {
		inline bool operator()(const MessageEvent& x, const MessageEvent& y) const {
			return x.ttlSeconds > y.ttlSeconds;
		}
	};

	typedef std::vector<MessageEvent> MessageEventQueue;
	MessageEventQueue _messageEventQueue;
	MessageEventComparator _messageEventQueueComp;
	double _timeSeconds = 0.0;
public:
	/**
	 * @brief Registers a console command to add messages from scripts or console
	 */
	void construct() override;
	/**
	 * @brief Initializes this component
	 * @sa @c shutdown()
	 */
	bool init() override;
	/**
	 * @brief The update method will remove outdated messages.
	 */
	void update(double deltaFrameSeconds);
	/**
	 * @brief Perform a cleanup of the component.
	 * @sa @c init()
	 */
	void shutdown() override;

	/**
	 * @brief Adds a message to the message queue
	 */
	void message(CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(2);

	/**
	 * @brief Iterates over all active messages and call the given functor with the remaining millis and the string of the message
	 * @note The oldest messages are coming first
	 * @note Call @c update() to get rid of outdated messages
	 */
	template<class FUNC>
	inline void visitMessages(FUNC&& func) const {
		for (const auto& m : _messageEventQueue) {
			func(m.ttlSeconds - _timeSeconds, m.msg);
		}
	}
};
