/**
 * @file
 */

#pragma once

#include "IMetricSender.h"
#include <string>
#include <mutex>
#include <cstdint>

#ifndef WIN32
#define SOCKET  int
#endif

struct sockaddr_in;

namespace metric {

class UDPMetricSender : public IMetricSender {
private:
	std::string _host;
	mutable SOCKET _socket;
	uint16_t _port = 0u;
	mutable struct sockaddr_in* _statsd;
	mutable std::mutex _connectionMutex;

	bool connect() const;
public:
	UDPMetricSender();
	bool send(const char* buffer) const override;

	/**
	 * Connects to the port and host given by the cvars @c metric_port and @c metric_host.
	 */
	bool init() override;
	void shutdown() override;
};

}
