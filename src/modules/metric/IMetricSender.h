/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"

namespace metric {

/**
 * @brief Interface for the metric sender
 * @ingroup Metric
 * @see Metric
 */
class IMetricSender : public core::IComponent {
public:
	virtual ~IMetricSender() {
	}

	virtual bool send(const char *buffer) const = 0;

	virtual bool init() override {
		return true;
	}

	virtual void shutdown() override {
	}
};

using IMetricSenderPtr = core::SharedPtr<IMetricSender>;

} // namespace metric
