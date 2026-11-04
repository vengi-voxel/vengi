/**
 * @file
 */

#include "Request.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "engine-config.h"
#include "io/BufferedReadWriteStream.h"
#include "system/Async.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#include "system/WinHttp.h"
#elif defined(__EMSCRIPTEN__)
#include "system/Emscripten.h"
#elif defined(__ANDROID__)
#include "system/Android.h"
#elif USE_CURL
#include "system/Curl.h"
#else
#include "system/Null.h"
#endif

namespace http {

bool Request::supported() {
	return http_supported();
}

Request::Request(const core::String &url, RequestType type) {
	_ctx._type = type;
	_ctx._url = url;
	const core::VarDef httpTimeout(cfg::HttpTimeout, 5, N_("HTTP timeout"), N_("The timeout for HTTP requests in seconds"));
	_ctx._timeoutSecond = core::Var::registerVar(httpTimeout)->intVal();
	const core::VarDef httpConnectTimeout(cfg::HttpConnectTimeout, 1, N_("HTTP connect timeout"), N_("The timeout for HTTP connection in seconds"));
	_ctx._connectTimeoutSecond = core::Var::registerVar(httpConnectTimeout)->intVal();
	_ctx._userAgent = "vengi/" PROJECT_VERSION;
}

bool Request::setBody(const core::String &body) {
	return setBody(body.c_str(), body.size());
}

bool Request::setBody(const void *data, size_t size) {
	core_assert(_ctx._type == RequestType::POST || _ctx._type == RequestType::PATCH);
	if (_ctx._type != RequestType::POST && _ctx._type != RequestType::PATCH) {
		return false;
	}
	core::Buffer<uint8_t> body;
	if (size > 0) {
		body.append((const uint8_t *)data, size);
	}
	_ctx._body = core::move(body);
	return true;
}

void Request::addHeader(const core::String &key, const core::String &value) {
	_ctx._headers.put(key, value);
}

void Request::noCache() {
	addHeader("Cache-Control", "no-cache");
}

bool Request::execute(io::WriteStream &stream, int *statusCode, Headers *outheaders) {
	if (statusCode) {
		*statusCode = 0;
	}
	Log::debug("Starting http request for %s", _ctx._url.c_str());
	return http_request(stream, statusCode, outheaders, _ctx);
}

void detail::executeRequest(AsyncRequest &request) {
	io::BufferedReadWriteStream stream;
	request.response.success =
		http_request(stream, &request.response.statusCode, &request.response.headers, request.context);
	if (stream.size() > 0) {
		request.response.body.append((const uint8_t *)stream.getBuffer(), (size_t)stream.size());
	}
}

} // namespace http
