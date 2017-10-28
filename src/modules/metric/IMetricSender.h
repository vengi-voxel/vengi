/**
 * @file
 */

#pragma once

#include <memory>

namespace metric {

class IMetricSender {
public:
	virtual ~IMetricSender() {
	}

	virtual bool send(const char* buffer) const = 0;

	virtual bool init() {
		return true;
	}

	virtual void shutdown() {
	}
};

using IMetricSenderPtr = std::shared_ptr<IMetricSender>;

}
