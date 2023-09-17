/**
 * @file
 */

#include "Request.h"
#include "app/App.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Var.h"
#include "engine-config.h"
#if __WINDOWS__
#define WIN32_LEAN_AND_MEAN (1)
#include <windows.h>
#include <winhttp.h>
#elif USE_CURL
#include <curl/curl.h>
#endif

namespace http {

#ifdef __WINDOWS__
static void printLastError(const char *ctx) {
	DWORD errnum = ::GetLastError();
	LPTSTR errmsg = nullptr;
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errmsg, 0, nullptr);
	if (errmsg == nullptr) {
		Log::error("%s: %d - Unknown error", ctx, errnum);
	} else {
		Log::error("%s: %d - %s", ctx, errnum, errmsg);
		::LocalFree(errmsg);
	}
}
#elif USE_CURL
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	return ((io::WriteStream *)userp)->write(contents, size * nmemb);
}
#endif

bool Request::supported() {
#if __WINDOWS__
	return true;
#elif USE_CURL
	return true;
#else
	return false;
#endif
}

bool Request::request(const core::String &url, io::WriteStream &stream) {
#if __WINDOWS__
	// Initialize WinHTTP and create a session
	HINTERNET hSession =
		WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession == nullptr) {
		printLastError("Failed to create session for http download");
		return false;
	}

	// Set the connection timeout
	const DWORD httpTimeoutMillis = core::Var::get(cfg::HttpTimeout, "5")->intVal() * 1000;
	DWORD dwResolveTimeout = httpTimeoutMillis;
	DWORD dwConnectTimeout = core::Var::get(cfg::HttpConnectTimeout, "5")->intVal() * 1000;
	DWORD dwSendTimeout = httpTimeoutMillis;
	DWORD dwReceiveTimeout = httpTimeoutMillis;
	if (!WinHttpSetTimeouts(hSession, dwResolveTimeout, dwConnectTimeout, dwSendTimeout, dwReceiveTimeout)) {
		printLastError("Failed to set http timeouts");
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::wstring urlw(url.begin(), url.end());
	// Open a connection to the remote server
	HINTERNET hConnect = WinHttpOpenRequest(hSession, L"GET", urlw.c_str(), nullptr, WINHTTP_NO_REFERER,
											WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_REFRESH);
	if (hConnect == nullptr) {
		printLastError("Failed to connect to url");
		WinHttpCloseHandle(hSession);
		return false;
	}

	// add request headers
	const wchar_t *reqHeaders = L"User-Agent: Mozilla/5.0\r\n"
								L"Connection: keep-alive\r\n"
								L"Accept-Encoding: gzip, deflate";
	if (!WinHttpAddRequestHeaders(hConnect, reqHeaders, -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
		Log::warn("Failed to add request headers to url: %s", url.c_str());
	}

	// Send the request
	if (!WinHttpSendRequest(hConnect, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		printLastError("Failed to send request");
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	if (!WinHttpReceiveResponse(hConnect, nullptr)) {
		printLastError("Failed to receive response");
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	DWORD dwStatusCode = 0;
	DWORD dwSize = sizeof(dwStatusCode);
	WinHttpQueryHeaders(hConnect, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &dwStatusCode,
						&dwSize, nullptr);
	if (dwStatusCode != 200) {
		Log::warn("Failed to download url: %s with status code: %d", url.c_str(), dwStatusCode);
	}

	// Read and save the response data
	DWORD bytesRead;
	BYTE buffer[4096];
	while (WinHttpReadData(hConnect, buffer, sizeof(buffer), &bytesRead)) {
		// Write the 'bytesRead' bytes from the buffer
		stream.write(buffer, bytesRead);
	}
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return true;
#elif USE_CURL
	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		return false;
	}

	const core::String userAgent = app::App::getInstance()->appname() + " " PROJECT_VERSION;
	Log::debug("Starting http request for %s", url.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, core::Var::get(cfg::HttpConnectTimeout, "5")->intVal());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, core::Var::get(cfg::HttpTimeout, "5")->intVal());
	curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
	const CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res == CURLE_OK;
#endif
	return false;
}

} // namespace http
