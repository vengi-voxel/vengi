/**
 * @file
 */

#include "tb_msg.h"
#include "tb_system.h"
#include <stddef.h>

namespace tb {

/** List of all delayed messages */
TBLinkListOf<TBMessageLink> g_all_delayed_messages;

/** List of all nondelayed messages. */
TBLinkListOf<TBMessageLink> g_all_normal_messages;

TBMessage::TBMessage(const TBID &message, TBMessageData *data, double fireTimeMs, TBMessageHandler *mh)
	: message(message), data(data), fire_time_ms(fireTimeMs), mh(mh) {
}

TBMessage::~TBMessage() {
	delete data;
}

TBMessageHandler::TBMessageHandler() {
}

TBMessageHandler::~TBMessageHandler() {
	deleteAllMessages();
}

bool TBMessageHandler::postMessageDelayed(const TBID &message, TBMessageData *data, uint32_t delayInMs) {
	return postMessageOnTime(message, data, TBSystem::getTimeMS() + (double)delayInMs);
}

bool TBMessageHandler::postMessageOnTime(const TBID &message, TBMessageData *data, double fireTime) {
	if (TBMessage *msg = new TBMessage(message, data, fireTime, this)) {
		// Find the message that is already in the list that should fire later, so we can
		// insert msg just before that. (Always keep the list ordered after fire time)

		// NOTE: If another message is added during OnMessageReceived, it might or might not be fired
		// in the right order compared to other delayed messages, depending on if it's inserted before or
		// after the message being processed!

		TBMessage *later_msg = nullptr;
		TBMessageLink *link = g_all_delayed_messages.getFirst();
		while (link != nullptr) {
			TBMessage *msg_in_list = static_cast<TBMessage *>(link);
			if (msg_in_list->fire_time_ms > msg->fire_time_ms) {
				later_msg = msg_in_list;
				break;
			}
			link = link->getNext();
		}

		// Add it to the global list in the right order.
		if (later_msg != nullptr) {
			g_all_delayed_messages.addBefore(msg, later_msg);
		} else {
			g_all_delayed_messages.addLast(msg);
		}

		// Add it to the list in messagehandler.
		m_messages.addLast(msg);

		// If we added it first and there's no normal messages, the next fire time has
		// changed and we have to reschedule the timer.
		if ((g_all_normal_messages.getFirst() == nullptr) && g_all_delayed_messages.getFirst() == msg) {
			TBSystem::rescheduleTimer(msg->fire_time_ms);
		}
		return true;
	}
	return false;
}

bool TBMessageHandler::postMessage(const TBID &message, TBMessageData *data) {
	if (TBMessage *msg = new TBMessage(message, data, 0, this)) {
		g_all_normal_messages.addLast(msg);
		m_messages.addLast(msg);

		// If we added it and there was no messages, the next fire time has
		// changed and we have to reschedule the timer.
		if (g_all_normal_messages.getFirst() == msg) {
			TBSystem::rescheduleTimer(0);
		}
		return true;
	}
	return false;
}

TBMessage *TBMessageHandler::getMessageByID(const TBID &message) {
	TBLinkListOf<TBMessage>::Iterator iter = m_messages.iterateForward();
	while (TBMessage *msg = iter.getAndStep()) {
		if (msg->message == message) {
			return msg;
		}
	}
	return nullptr;
}

void TBMessageHandler::deleteMessage(TBMessage *msg) {
	core_assert(msg->mh == this); // This is not the message handler owning the message!

	// Remove from global list (g_all_delayed_messages or g_all_normal_messages)
	if (g_all_delayed_messages.containsLink(msg)) {
		g_all_delayed_messages.remove(msg);
	} else if (g_all_normal_messages.containsLink(msg)) {
		g_all_normal_messages.remove(msg);
	}

	// Remove from local list
	m_messages.remove(msg);

	delete msg;

	// Note: We could call TBSystem::RescheduleTimer if we think that deleting
	// this message changed the time for the next message.
}

void TBMessageHandler::deleteAllMessages() {
	while (TBMessage *msg = m_messages.getFirst()) {
		deleteMessage(msg);
	}
}

// static
void TBMessageHandler::processMessages() {
	// Handle delayed messages
	TBLinkListOf<TBMessageLink>::Iterator iter = g_all_delayed_messages.iterateForward();
	while (TBMessage *msg = static_cast<TBMessage *>(iter.getAndStep())) {
		if (TBSystem::getTimeMS() >= msg->fire_time_ms) {
			// Remove from global list
			g_all_delayed_messages.remove(msg);
			// Remove from local list
			msg->mh->m_messages.remove(msg);

			msg->mh->onMessageReceived(msg);

			delete msg;
		} else {
			break; // Since the list is sorted, all remaining messages should fire later
		}
	}

	// Handle normal messages
	iter = g_all_normal_messages.iterateForward();
	while (TBMessage *msg = static_cast<TBMessage *>(iter.getAndStep())) {
		// Remove from global list
		g_all_normal_messages.remove(msg);
		// Remove from local list
		msg->mh->m_messages.remove(msg);

		msg->mh->onMessageReceived(msg);

		delete msg;
	}
}

// static
double TBMessageHandler::getNextMessageFireTime() {
	if (g_all_normal_messages.getFirst() != nullptr) {
		return 0;
	}

	if (g_all_delayed_messages.getFirst() != nullptr) {
		TBMessage *first_delayed_msg = static_cast<TBMessage *>(g_all_delayed_messages.getFirst());
		return first_delayed_msg->fire_time_ms;
	}

	return TB_NOT_SOON;
}

} // namespace tb
