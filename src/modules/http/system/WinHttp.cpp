/**
 * @file
 */

#include "WinHttp.h"
#include "core/Log.h"
#include "core/ArrayLength.h"

#define WIN32_LEAN_AND_MEAN (1)
#include <sstream>
#include <string>
#include <windows.h>
#include <winhttp.h>

namespace http {

static void printLastError(const char *ctx) {
	DWORD errnum = ::GetLastError();
	char buffer[512] = "";
	if (::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
						 GetModuleHandleA("winhttp.dll"), errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer,
						 sizeof(buffer), NULL) == 0) {
		Log::error("%s: %d - Unknown error", ctx, (int)errnum);
	} else {
		Log::error("%s: %d - %s", ctx, (int)errnum, buffer);
	}
}

static std::wstring s2ws(const std::string &str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

static std::string ws2s(const std::wstring &wstr) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

bool http_request(io::WriteStream &stream, int *statusCode, Headers *outheaders,
				  RequestContext &ctx) {
	// Initialize WinHTTP and create a session
	HINTERNET hSession =
		WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession == nullptr) {
		printLastError("Failed to create session for http download");
		return false;
	}

	// Set the connection timeout
	DWORD dwResolveTimeout = ctx._timeoutSecond * 1000;
	DWORD dwConnectTimeout = ctx._connectTimeoutSecond * 1000;
	DWORD dwSendTimeout = dwResolveTimeout;
	DWORD dwReceiveTimeout = dwResolveTimeout;
	if (!WinHttpSetTimeouts(hSession, dwResolveTimeout, dwConnectTimeout, dwSendTimeout, dwReceiveTimeout)) {
		printLastError("Failed to set http timeouts");
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::wstring urlw = s2ws(ctx._url.c_str());
	URL_COMPONENTS url_components = {};
	wchar_t scheme[32];
	wchar_t hostname[128];
	wchar_t url_path[4096];

	/* Convert the URL to its components. */
	url_components.dwStructSize = sizeof(url_components);
	url_components.lpszScheme = scheme;
	url_components.dwSchemeLength = lengthof(scheme);
	url_components.lpszHostName = hostname;
	url_components.dwHostNameLength = lengthof(hostname);
	url_components.lpszUrlPath = url_path;
	url_components.dwUrlPathLength = lengthof(url_path);
	url_components.nPort = INTERNET_DEFAULT_HTTP_PORT;
	if (!WinHttpCrackUrl(urlw.c_str(), 0, 0, &url_components)) {
		printLastError("Failed to parse url");
		WinHttpCloseHandle(hSession);
		return false;
	}

	/* Create the HTTP connection. */
	HINTERNET hConnection = WinHttpConnect(hSession, url_components.lpszHostName, url_components.nPort, 0);
	if (hConnection == nullptr) {
		printLastError("Failed to connect");
		WinHttpCloseHandle(hSession);
		return false;
	}

	const wchar_t *method = ctx._type == RequestType::GET ? L"GET" : L"POST";
	HINTERNET hRequest = WinHttpOpenRequest(hConnection, method, url_components.lpszUrlPath, nullptr,
											WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
											url_components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
	if (hRequest == nullptr) {
		printLastError("Failed to create request");
		WinHttpCloseHandle(hSession);
		WinHttpCloseHandle(hConnection);
		return false;
	}

	// add request headers

	std::wstring reqHeaders;
	if (!ctx._userAgent.empty()) {
		reqHeaders += L"User-Agent: ";
		reqHeaders += s2ws(ctx._userAgent.c_str());
		reqHeaders += L"\r\n";
	}
	for (const auto &entry : ctx._headers) {
		reqHeaders += s2ws(entry->first.c_str());
		reqHeaders += L": ";
		reqHeaders += s2ws(entry->second.c_str());
		reqHeaders += L"\r\n";
	}

	SIZE_T len = reqHeaders.size();
	if (len > 0 && !WinHttpAddRequestHeaders(hRequest, reqHeaders.c_str(), DWORD(len),
											 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
		Log::error("Failed to add request headers to url: %s", ctx._url.c_str());
		printLastError("Failed to add request headers");
	}

	// Send the request
	int maxRedirects = 3;
	bool requestState = false;
	if (ctx._type == RequestType::GET) {
		while (!requestState) {
			requestState =
				WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
			if (!requestState && GetLastError() == ERROR_WINHTTP_RESEND_REQUEST) {
				if (maxRedirects <= 0) {
					break;
				}
				--maxRedirects;
				continue;
			}
			if (!requestState) {
				Log::error("Failed to send request with error %d", (int)GetLastError());
				break;
			}
		}
	} else {
		while (!requestState) {
			requestState = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)ctx._body.c_str(),
											  ctx._body.size(), ctx._body.size(), 0);
			if (!requestState && GetLastError() == ERROR_WINHTTP_RESEND_REQUEST) {
				if (maxRedirects <= 0) {
					break;
				}
				--maxRedirects;
				continue;
			}
			if (!requestState) {
				Log::error("Failed to send request with error %d", (int)GetLastError());
				break;
			}
		}
	}
	WinHttpReceiveResponse(hRequest, nullptr);
	if (!WinHttpQueryDataAvailable(hRequest, nullptr)) {
		printLastError("Failed to query available data");
	}

	DWORD dwStatusCode = 0;
	DWORD dwSize = sizeof(dwStatusCode);
	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
							 WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
		printLastError("Failed to query status code");
	}
	const char *requestTypeStr = ctx._type == RequestType::GET ? "GET" : "POST";
	Log::debug("Http request for url: %s (%s) with status code: %d", ctx._url.c_str(), requestTypeStr, (int)dwStatusCode);
	if (statusCode) {
		*statusCode = (int)dwStatusCode;
	}

	if (outheaders) {
		DWORD headerLength = sizeof(DWORD);
		if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, nullptr,
								 &headerLength, WINHTTP_NO_HEADER_INDEX) &&
			GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			wchar_t *rawHeader = new wchar_t[headerLength / sizeof(wchar_t)];
			ZeroMemory(rawHeader, headerLength);
			if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, rawHeader,
									&headerLength, WINHTTP_NO_HEADER_INDEX)) {
				std::wstring headerStr(rawHeader);
				std::wstringstream headerStream(headerStr);
				std::wstring line;
				while (std::getline(headerStream, line)) {
					size_t delimiter = line.find(L": ");
					if (delimiter != std::wstring::npos) {
						const std::string &key = ws2s(line.substr(0, delimiter));
						const std::string &value = ws2s(line.substr(delimiter + 2));
						outheaders->put(key.c_str(), value.c_str());
					}
				}
			}
			delete[] rawHeader;
		}
	}

	// Read and save the response data
	DWORD bytesRead;
	BYTE buffer[4096];
	// Reserve buffer space up-front if Content-Length header is available
	DWORD dwContentLength = 0;
	DWORD dwContentLengthSize = sizeof(dwContentLength);
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
							WINHTTP_HEADER_NAME_BY_INDEX, &dwContentLength, &dwContentLengthSize, WINHTTP_NO_HEADER_INDEX)) {
		if (dwContentLength > 0) {
			stream.reserve((int)dwContentLength);
		}
	}
	while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead)) {
		// Write the 'bytesRead' bytes from the buffer
		if (bytesRead == 0) {
			break;
		}
		stream.write(buffer, bytesRead);
	}
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hSession);
	WinHttpCloseHandle(hConnection);
	return true;
}

} // namespace http
