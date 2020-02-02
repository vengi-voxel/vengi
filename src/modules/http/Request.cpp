/**
 * @file
 */

#include "Request.h"
#include "Url.h"
#include "core/App.h"
#include "core/Log.h"
#include "core/ArrayLength.h"
#include <string.h>
#include "Network.cpp.h"

namespace http {

Request::Request(const Url& url, HttpMethod method) :
		_url(url), _socketFD(INVALID_SOCKET), _method(method) {
	_headers.put(header::USER_AGENT, core::App::getInstance()->appname().c_str());
	_headers.put(header::CONNECTION, "close");
	// TODO:
	// _headers.put(header::KEEP_ALIVE, "timeout=15");
	// _headers.put(header::CONNECTION, "Keep-Alive");
	_headers.put(header::ACCEPT_ENCODING, "gzip, deflate");
	accept("*/*");
	if (HttpMethod::POST == method && !_url.query.empty()) {
		contentType("application/x-www-form-urlencoded");
		body(_url.query.c_str());
	}
}

ResponseParser Request::failed() {
	closesocket(_socketFD);
	_socketFD = INVALID_SOCKET;
	return ResponseParser(nullptr, 0u);
}

ResponseParser Request::execute() {
	if (!_url.valid()) {
		Log::error("Invalid url given");
		return ResponseParser(nullptr, 0u);
	}

	if (!networkInit()) {
		Log::error("Failed to initialize the network");
		return ResponseParser(nullptr, 0u);
	}

	_socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socketFD == INVALID_SOCKET) {
		Log::error("Failed to initialize the socket");
		network_cleanup();
		return ResponseParser(nullptr, 0u);
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo* results = nullptr;
	const int ret = getaddrinfo(_url.hostname.c_str(), nullptr, &hints, &results);
	if (ret != 0) {
		Log::error("Failed to resolve host for %s", _url.hostname.c_str());
		return failed();
	}
	const struct sockaddr_in* host_addr = (const struct sockaddr_in*) results->ai_addr;
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(_url.port);
	memcpy(&sin.sin_addr, &host_addr->sin_addr, sizeof(sin.sin_addr));
	freeaddrinfo(results);
	if (connect(_socketFD, (const struct sockaddr *)&sin, sizeof(sin)) == -1) {
		Log::error("Failed to connect to %s:%i", _url.hostname.c_str(), _url.port);
		return failed();
	}

	char headers[1024];
	if (!buildHeaderBuffer(headers, lengthof(headers), _headers)) {
		Log::error("Failed to assemble request header");
		return failed();
	}

	char message[4096];
	if (_method == HttpMethod::GET) {
		if (SDL_snprintf(message, sizeof(message),
				"GET %s%s%s HTTP/1.1\r\n"
				"Host: %s\r\n"
				"%s"
				"\r\n",
				_url.path.c_str(),
				(_url.query.empty() ? "" : "?"),
				_url.query.c_str(),
				_url.hostname.c_str(),
				headers) >= lengthof(message)) {
			Log::error("Failed to assemble request");
			return failed();
		}
	} else if (_method == HttpMethod::POST) {
		if (SDL_snprintf(message, sizeof(message),
				"POST %s HTTP/1.1\r\n"
				"Host: %s\r\n"
				"%s"
				"\r\n"
				"%s",
				_url.path.c_str(),
				_url.hostname.c_str(),
				headers,
				_body) >= lengthof(message)) {
			Log::error("Failed to assemble request");
			return failed();
		}
	} else {
		Log::error("Unsupported method");
		return failed();
	}

	size_t sent = 0u;
	while (sent < strlen(message)) {
		const int ret = send(_socketFD, message + sent, strlen(message) - sent, 0);
		if (ret < 0) {
			Log::error("Failed to perform http request to %s", _url.url.c_str());
			return failed();
		}
		sent += ret;
	}

	uint8_t *response = nullptr;
	constexpr const int BUFFERSIZE = 1024 * 1024;
	uint8_t *recvBuf = (uint8_t*)SDL_malloc(BUFFERSIZE);
	int32_t receivedLength = 0;
	size_t totalReceivedLength = 0u;
	while ((receivedLength = recv(_socketFD, (char*)recvBuf, BUFFERSIZE, 0)) > 0) {
		response = (uint8_t*)SDL_realloc(response, totalReceivedLength + receivedLength);
		memcpy(response + totalReceivedLength, recvBuf, receivedLength);
		totalReceivedLength += receivedLength;
		Log::trace("received data: %i", (int)receivedLength);
	}
	SDL_free(recvBuf);
	if (receivedLength < 0) {
		Log::error("Failed to read http response from %s", _url.url.c_str());
		return failed();
	}

	closesocket(_socketFD);
	network_cleanup();

	ResponseParser parser(response, totalReceivedLength);
	const char *encoding;
	if (parser.headers.get(header::CONTENT_ENCODING, encoding)) {
		// TODO: gunzip
	}
	return parser;
}

}
