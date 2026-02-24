/**
 * @file
 */

#include "Request.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "engine-config.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#include "system/WinHttp.h"
#elif EMSCRIPTEN
#include "system/Emscripten.h"
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
	_ctx._timeoutSecond = core::Var::registerVar(core::VarDef(cfg::HttpTimeout, 5))->intVal();
	_ctx._connectTimeoutSecond = core::Var::registerVar(core::VarDef(cfg::HttpConnectTimeout, 1))->intVal();
	_ctx._userAgent = "vengi/" PROJECT_VERSION;
}

bool Request::setBody(const core::String &body) {
	core_assert(_ctx._type == RequestType::POST);
	if (_ctx._type != RequestType::POST) {
		return false;
	}
	_ctx._body = body;
	return true;
}

void Request::addHeader(const core::String &key, const core::String &value) {
	_ctx._headers.put(key, value);
}

void Request::noCache() {
	addHeader("Cache-Control", "no-cache");
}

bool Request::execute(io::WriteStream &stream, int *statusCode, Headers *outheaders) {
	Log::debug("Starting http request for %s", _ctx._url.c_str());
	return http_request(stream, statusCode, outheaders, _ctx);
}

} // namespace http
