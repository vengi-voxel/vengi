/**
 * @file
 */

#include "Http.h"
#include "Url.h"
#include "Request.h"

namespace http {

ResponseParser GET(const char *url) {
	Url u(url);
	Request request(u, HttpMethod::GET);
	return request.execute();
}

ResponseParser POST(const char *url, const char *body) {
	Url u(url);
	Request request(u, HttpMethod::POST);
	if (body != nullptr) {
		request.body(body);
	}
	return request.execute();
}

}
