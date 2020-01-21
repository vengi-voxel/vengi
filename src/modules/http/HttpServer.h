/**
 * @file
 */

#pragma once

#include "HttpMethod.h"
#include "HttpResponse.h"
#include "HttpStatus.h"
#include "RequestParser.h"
#include "Network.h"
#include "HttpHeader.h"
#include "HttpQuery.h"
#include "core/collection/Map.h"
#include "core/metric/Metric.h"
#include <stdint.h>
#include <functional>
#include <list>
#include <memory>

namespace http {

class RequestParser;

class HttpServer {
public:
	using RouteCallback = std::function<void(const RequestParser& query, HttpResponse* response)>;
private:
	SOCKET _socketFD;
	fd_set _readFDSet;
	fd_set _writeFDSet;
	using Routes = core::Map<const char*, RouteCallback, 8, core::hashCharPtr, core::hashCharCompare>;
	core::Map<int, const char*, 8, std::hash<int>> _errorPages;
	Routes _routes[2];
	size_t _maxRequestBytes = 1 * 1024 * 1024;
	metric::MetricPtr _metric;

	struct Client {
		Client();
		SOCKET socket;

		uint8_t *request = nullptr;
		size_t requestLength = 0u;

		char* response = nullptr;
		size_t responseLength = 0u;
		size_t alreadySent = 0u;

		void setResponse(char* responseBuf, size_t responseBufLength);
		bool finished() const;
	};

	using ClientSockets = std::list<Client>;
	using ClientSocketsIter = ClientSockets::iterator;
	ClientSockets _clientSockets;

	ClientSocketsIter closeClient (ClientSocketsIter& i);

	void metric(HttpStatus status) const;

	bool route(const RequestParser& request, HttpResponse& response);
	void assembleResponse(Client& client, const HttpResponse& response);
	void assembleError(Client& client, HttpStatus status);
	bool sendMessage(Client& client);

	Routes* getRoutes(HttpMethod method);

public:
	HttpServer(const metric::MetricPtr& metric);
	~HttpServer();

	void setMaxRequestSize(size_t maxBytes);

	/**
	 * @param[in] body The status code body. The pointer is copied and then released by the server.
	 */
	void setErrorText(HttpStatus status, const char *body);

	bool init(int16_t port = 8080);
	bool update();
	void shutdown();

	void registerRoute(HttpMethod method, const char *path, RouteCallback callback);
	bool unregisterRoute(HttpMethod method, const char *path);
};

inline void HttpServer::setMaxRequestSize(size_t maxBytes) {
	_maxRequestBytes = maxBytes;
}


typedef std::shared_ptr<HttpServer> HttpServerPtr;

}
