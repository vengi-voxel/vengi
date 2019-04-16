/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include <string>
#include <stdint.h>
#include <queue>
#include <vector>
#include <SDL.h>

/**
 * @brief Class that implements messages with lifetime. The messages are removed once the got old enough.
 *
 * This can e.g. be used to display hud messages
 */
class MessageQueue : public core::IComponent {
private:
	struct MessageEvent {
		MessageEvent(uint64_t _ttl, const std::string& _msg) :
				ttl(_ttl), msg(_msg) {
		}
		uint64_t ttl;
		std::string msg;
	};

	struct MessageEventComparator {
		inline bool operator()(const MessageEvent& x, const MessageEvent& y) const {
			return x.ttl > y.ttl;
		}
	};

	typedef std::vector<MessageEvent> MessageEventQueue;
	MessageEventQueue _messageEventQueue;
	MessageEventComparator _messageEventQueueComp;
	uint64_t _time = 0u;
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
	void update(uint64_t dt);
	/**
	 * @brief Perform a cleanup of the component.
	 * @sa @c init()
	 */
	void shutdown() override;

	/**
	 * @brief Adds a message to the message queue
	 */
	void message(SDL_PRINTF_FORMAT_STRING const char *msg, ...) SDL_PRINTF_VARARG_FUNC(2);

	/**
	 * @brief Iterates over all active messages and call the given functor with the remaining millis and the string of the message
	 * @note The oldest messages are coming first
	 * @note Call @c update() to get rid of outdated messages
	 */
	template<class FUNC>
	inline void visitMessages(FUNC&& func) const {
		for (const auto& m : _messageEventQueue) {
			func(m.ttl - _time, m.msg);
		}
	}
};
