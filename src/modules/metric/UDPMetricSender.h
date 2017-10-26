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
	const std::string _host;
	mutable SOCKET _socket;
	const uint16_t _port;
	mutable struct sockaddr_in* _statsd;
	mutable std::mutex _connectionMutex;

	bool connect() const;
public:
	UDPMetricSender(uint16_t port = 8125, const std::string& host = "127.0.0.1");
	bool send(const char* buffer) const override;

	bool init() override;
	void shutdown() override;
};

}
