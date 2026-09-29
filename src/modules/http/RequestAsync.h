/**
 * @file
 */

#pragma once

#include "core/Function.h"
#include "http/Request.h"
#include "http/Response.h"

namespace http {

using ResponseCallback = core::Function<void(const Response &)>;

/**
 * @brief Run @p request on the thread pool and invoke @p callback on the main thread.
 *
 * The callback is not called from the worker. It is queued and dispatched from
 * @c http::update(), which must be ticked on the main thread (e.g. from App::onFrame).
 *
 * @return A non-zero request id, or 0 if the request could not be scheduled.
 */
uint32_t requestAsync(Request &&request, ResponseCallback &&callback);

/**
 * @brief Convenience overload that builds a GET/POST/PATCH request for @p url.
 */
uint32_t requestAsync(const core::String &url, RequestType type, ResponseCallback &&callback);

/**
 * @brief Dispatch finished request callbacks on the main thread.
 * Call once per frame from the application main loop.
 */
void update();

/**
 * @brief Drop pending callbacks without invoking them (e.g. on app shutdown).
 */
void clearPending();

} // namespace http
