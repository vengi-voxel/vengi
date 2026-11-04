/**
 * @file
 */

#pragma once

#include "core/Function.h"
#include "http/Request.h"
#include "http/Response.h"

namespace http {

using RequestId = uint64_t;
using ResponseCallback = core::Function<void(const Response &)>;

/**
 * @brief Owns asynchronous requests. Destruction cancels outstanding callbacks.
 * @note Use on the application thread, including destruction and cancellation.
 * Submission via execute() is thread-safe. Request data is copied on submission.
 * Transports start and callbacks run only from http::update(),
 * including failures to start a transport. A callback may submit or cancel requests.
 */
class Client {
private:
	uint64_t _id;

public:
	Client();
	~Client();
	Client(const Client &) = delete;
	Client &operator=(const Client &) = delete;

	/** @return Nonzero on acceptance, zero if unavailable, shut down, or 64 requests are outstanding. */
	RequestId execute(const Request &request, ResponseCallback callback = {});
	/** @brief Suppress the callback and abort the transport where supported. */
	bool cancel(RequestId id);
	void cancelAll();
};

/** @brief Application lifecycle. update() never blocks waiting for network I/O. */
void init();
void update();
void shutdown();

} // namespace http
