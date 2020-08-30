/**
 * @file
 */

#include "HttpServer.h"
#include "RequestParser.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "Network.cpp.h"
#include "app/App.h"
#include <string.h>
#include <SDL_stdinc.h>

namespace http {

HttpServer::HttpServer(const metric::MetricPtr& metric) :
		_socketFD(INVALID_SOCKET), _metric(metric) {
	FD_ZERO(&_readFDSet);
	FD_ZERO(&_writeFDSet);
}

HttpServer::~HttpServer() {
	core_assert(_socketFD == INVALID_SOCKET);
}

void HttpServer::setErrorText(HttpStatus status, const char *body) {
	auto i = _errorPages.find((int)status);
	if (i != _errorPages.end()) {
		SDL_free((char*)i->value);
	}
	_errorPages.put((int)status, SDL_strdup(body));
}

HttpServer::Routes* HttpServer::getRoutes(HttpMethod method) {
	if (method == HttpMethod::GET) {
		return &_routes[0];
	} else /* if (method == HttpMethod::POST) */ {
		core_assert(method == HttpMethod::POST);
		return &_routes[1];
	}
}

void HttpServer::registerRoute(HttpMethod method, const char *path, const RouteCallback& callback) {
	Routes* routes = getRoutes(method);
	Log::info("Register callback for %s", path);
	routes->put(path, callback);
}

bool HttpServer::unregisterRoute(HttpMethod method, const char *path) {
	Routes* routes = getRoutes(method);
	return routes->remove(path);
}

bool HttpServer::init(int16_t port) {
	_socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socketFD == INVALID_SOCKET) {
		network_cleanup();
		return false;
	}
	struct sockaddr_in sin;
	SDL_memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	FD_ZERO(&_readFDSet);
	FD_ZERO(&_writeFDSet);

	int t = 1;
#ifdef _WIN32
	if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, (char*) &t, sizeof(t)) != 0) {
#else
	if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) != 0) {
#endif
		network_cleanup();
		closesocket(_socketFD);
		_socketFD = INVALID_SOCKET;
		return false;
	}

	if (bind(_socketFD, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		network_cleanup();
		closesocket(_socketFD);
		_socketFD = INVALID_SOCKET;
		return false;
	}

	if (listen(_socketFD, 5) < 0) {
		network_cleanup();
		closesocket(_socketFD);
		_socketFD = INVALID_SOCKET;
		return false;
	}

	networkNonBlocking(_socketFD);

	FD_SET(_socketFD, &_readFDSet);

	return true;
}

HttpServer::ClientSocketsIter HttpServer::closeClient(ClientSocketsIter& iter) {
	Client& client = *iter;
	const SOCKET clientSocket = client.socket;
	FD_CLR(clientSocket, &_readFDSet);
	FD_CLR(clientSocket, &_writeFDSet);
	closesocket(clientSocket);
	client.socket = INVALID_SOCKET;
	SDL_free(client.request);
	SDL_free(client.response);
	return _clientSockets.erase(iter);
}

bool HttpServer::update() {
	core_trace_scoped(HttpServerUpdate);
	fd_set readFDsOut;
	fd_set writeFDsOut;

	SDL_memcpy(&readFDsOut, &_readFDSet, sizeof(readFDsOut));
	SDL_memcpy(&writeFDsOut, &_writeFDSet, sizeof(writeFDsOut));

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	const int ready = select(FD_SETSIZE, &readFDsOut, &writeFDsOut, nullptr, &tv);
	if (ready < 0) {
		return false;
	}
	if (_socketFD != INVALID_SOCKET && FD_ISSET(_socketFD, &readFDsOut)) {
		const SOCKET clientSocket = accept(_socketFD, nullptr, nullptr);
		if (clientSocket != INVALID_SOCKET) {
			FD_SET(clientSocket, &_readFDSet);
			Client c;
			c.socket = clientSocket;
			_clientSockets.insert(c);
			networkNonBlocking(clientSocket);
		}
	}

	for (ClientSocketsIter i = _clientSockets.begin(); i != _clientSockets.end();) {
		Client& client = *i;
		const SOCKET clientSocket = client.socket;
		if (clientSocket == INVALID_SOCKET) {
			i = closeClient(i);
			continue;
		}

		if (FD_ISSET(clientSocket, &writeFDsOut)) {
			if (!sendMessage(client) || client.finished()) {
				i = closeClient(i);
			} else {
				++i;
			}
			continue;
		}

		if (!FD_ISSET(clientSocket, &readFDsOut)) {
			++i;
			continue;
		}

		constexpr const int BUFFERSIZE = 2048;
		uint8_t recvBuf[BUFFERSIZE];
		const network_return len = recv(clientSocket, (char*)recvBuf, BUFFERSIZE - 1, 0);
		if (len < 0) {
			i = closeClient(i);
			continue;
		}
		if (len == 0) {
			++i;
			continue;
		}

		client.request = (uint8_t*)SDL_realloc(client.request, client.requestLength + len);
		SDL_memcpy(client.request + client.requestLength, recvBuf, len);
		client.requestLength += len;

		if (client.requestLength == 0) {
			++i;
			continue;
		}

		// GET / HTTP/1.1\r\n\r\n
		if (client.requestLength < 18) {
			++i;
			continue;
		}

		if (SDL_memcmp(client.request, "GET", 3) != 0 && SDL_memcmp(client.request, "POST", 4) != 0) {
			FD_CLR(clientSocket, &_readFDSet);
			FD_CLR(clientSocket, &readFDsOut);
			assembleError(client, HttpStatus::NotImplemented);
			++i;
			continue;
		}

		if (client.requestLength > _maxRequestBytes) {
			FD_CLR(clientSocket, &_readFDSet);
			FD_CLR(clientSocket, &readFDsOut);
			assembleError(client, HttpStatus::InternalServerError);
			++i;
			continue;
		}

		uint8_t *mem = (uint8_t *)SDL_malloc(client.requestLength);
		SDL_memcpy(mem, client.request, client.requestLength);
		const RequestParser request(mem, client.requestLength);
		if (!request.valid()) {
			++i;
			continue;
		}

		FD_CLR(clientSocket, &_readFDSet);
		FD_CLR(clientSocket, &readFDsOut);

		HttpResponse response;
		if (!route(request, response)) {
			assembleError(client, HttpStatus::NotFound);
			++i;
			continue;
		}
		assembleResponse(client, response);
		if (response.freeBody) {
			SDL_free((char*)response.body);
		}
	}
	return true;
}

void HttpServer::assembleError(Client& client, HttpStatus status) {
	char buf[512];
	SDL_snprintf(buf, sizeof(buf),
			"HTTP/1.1 %i %s\r\n"
			"Connection: close\r\n"
			"Server: %s\r\n"
			"\r\n",
			(int)status,
			toStatusString(status),
			app::App::getInstance()->appname().c_str());

	const char *errorPage = "";
	_errorPages.get((int)status, errorPage);

	const size_t responseSize = SDL_strlen(errorPage) + SDL_strlen(buf);
	char *responseBuf = (char*)SDL_malloc(responseSize + 1);
	SDL_snprintf(responseBuf, responseSize + 1, "%s%s", buf, errorPage);
	client.setResponse(responseBuf, responseSize);
	metric(status);
	FD_SET(client.socket, &_writeFDSet);
}

void HttpServer::assembleResponse(Client& client, const HttpResponse& response) {
	char headers[2048];
	if (!buildHeaderBuffer(headers, lengthof(headers), response.headers)) {
		assembleError(client, HttpStatus::InternalServerError);
		return;
	}

	char buf[4096];
	const int headerSize = SDL_snprintf(buf, sizeof(buf),
			"HTTP/1.1 %i %s\r\n"
			"Content-length: %u\r\n"
			"%s"
			"\r\n",
			(int)response.status,
			toStatusString(response.status),
			(unsigned int)response.bodySize,
			headers);
	if (headerSize >= lengthof(buf)) {
		assembleError(client, HttpStatus::InternalServerError);
		return;
	}

	const size_t responseSize = response.bodySize + SDL_strlen(buf);
	char *responseBuf = (char*)SDL_malloc(responseSize);
	SDL_memcpy(responseBuf, buf, headerSize);
	SDL_memcpy(responseBuf + headerSize, response.body, response.bodySize);
	client.setResponse(responseBuf, responseSize);
	Log::trace("Response buffer of size %i", (int)responseSize);
	metric(response.status);
	FD_SET(client.socket, &_writeFDSet);
}

void HttpServer::metric(HttpStatus status) const {
	char buf[8];
	SDL_snprintf(buf, sizeof(buf), "%u", (uint32_t)status);
	_metric->count("http.request", 1, {{"status", buf}});
}

bool HttpServer::sendMessage(Client& client) {
	core_assert(client.response != nullptr);
	int remaining = (int)client.responseLength - (int)client.alreadySent;
	if (remaining <= 0) {
		return false;
	}
	const char* p = client.response + client.alreadySent;
	const network_return sent = ::send(client.socket, p, remaining, 0);
	if (sent < 0) {
		Log::debug("Failed to send to the client");
		return false;
	}
	if (sent == 0) {
		return true;
	}
	remaining -= sent;
	client.alreadySent += sent;
	return remaining > 0;
}

bool HttpServer::route(const RequestParser& request, HttpResponse& response) {
	Routes* routes = getRoutes(request.method);
	Log::trace("lookup for %s", request.path);
	auto i = routes->find(request.path);
	if (i == routes->end()) {
		// check if there is at least one other /
		if (SDL_strchr(&request.path[1], '/')) {
			char *path = SDL_strdup(request.path);
			char *cut;
			for (;;) {
				cut = SDL_strrchr(path, '/');
				if (cut == path) {
					break;
				}
				*cut = '\0';
				Log::trace("lookup for %s", path);
				i = routes->find(path);
				if (i != routes->end()) {
					break;
				}
			}
			SDL_free(path);
		} else {
			Log::debug("No route found for '%s'", request.path);
			return false;
		}
	}
	if (i == routes->end()) {
		Log::debug("No route found for '%s'", request.path);
		return false;
	}
	response.headers.put(header::CONTENT_TYPE, http::mimetype::TEXT_PLAIN);
	response.headers.put(header::CONNECTION, "close");
	response.headers.put(header::SERVER, app::App::getInstance()->appname().c_str());
	// TODO urldecode of request data
	//core::string::urlDecode(request.query);
	i->value(request, &response);
	return true;
}

void HttpServer::shutdown() {
	const size_t l = lengthof(_routes);
	for (size_t i = 0; i < l; ++i) {
		_routes[i].clear();
	}
	for (ClientSocketsIter i = _clientSockets.begin(); i != _clientSockets.end();) {
		i = closeClient(i);
	}

	for (auto i : _errorPages) {
		SDL_free((char*)i->second);
	}
	_errorPages.clear();

	FD_ZERO(&_readFDSet);
	FD_ZERO(&_writeFDSet);
	closesocket(_socketFD);
	_socketFD = INVALID_SOCKET;
	network_cleanup();
}

HttpServer::Client::Client() :
		socket(INVALID_SOCKET) {
}

void HttpServer::Client::setResponse(char* responseBuf, size_t responseBufLength) {
	core_assert(response == nullptr);
	response = responseBuf;
	responseLength = responseBufLength;
	alreadySent = 0u;
}

bool HttpServer::Client::finished() const {
	if (response == nullptr) {
		return false;
	}
	return responseLength == alreadySent;
}

}
