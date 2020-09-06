/**
 * @file
 */

#include "RequestParser.h"
#include "core/StringUtil.h"
#include "HttpHeader.h"

namespace http {

RequestParser& RequestParser::operator=(RequestParser &&other) noexcept {
	if (&other != this) {
		Super::operator=(std::move(other));
		query = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.query);
		other.query.clear();
		method = other.method;
		path = HTTP_PARSER_NEW_BASE(other.path);
	}
	return *this;
}

RequestParser::RequestParser(RequestParser &&other) noexcept :
		Super(std::move(other)) {
	query = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.query);
	other.query.clear();
	method = other.method;
	path = HTTP_PARSER_NEW_BASE(other.path);
}

RequestParser& RequestParser::operator=(const RequestParser& other) {
	if (&other != this) {
		Super::operator=(other);
		query = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.query);
		method = other.method;
		path = HTTP_PARSER_NEW_BASE(other.path);
	}
	return *this;
}

RequestParser::RequestParser(const RequestParser &other) :
		Super(other) {
	query = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.query);
	method = other.method;
	path = HTTP_PARSER_NEW_BASE(other.path);
}

RequestParser::RequestParser(uint8_t* requestBuffer, size_t requestBufferSize)
		: Super(requestBuffer, requestBufferSize) {
	if (buf == nullptr || bufSize == 0) {
		return;
	}
	char *bufPos = (char *)buf;

	char *statusLine = getHeaderLine(&bufPos);
	if (statusLine == nullptr) {
		return;
	}
	char *methodStr = core::string::getBeforeToken(&statusLine, " ", remainingBufSize(bufPos));
	if (methodStr == nullptr) {
		return;
	}

	if (!SDL_strcmp(methodStr, "GET")) {
		method = HttpMethod::GET;
	} else if (!SDL_strcmp(methodStr, "POST")) {
		method = HttpMethod::POST;
	} else {
		method = HttpMethod::NOT_SUPPORTED;
		return;
	}
	char* request = core::string::getBeforeToken(&statusLine, " ", remainingBufSize(statusLine));
	if (request == nullptr) {
		return;
	}
	protocolVersion = statusLine;
	if (protocolVersion == nullptr) {
		return;
	}

	path = core::string::getBeforeToken(&request, "?", remainingBufSize(request));
	if (path == nullptr) {
		path = request;
	} else {
		char *queryString = request;
		bool last = false;
		for (;;) {
			char *paramValue = core::string::getBeforeToken(&queryString, "&", remainingBufSize(queryString));
			if (paramValue == nullptr) {
				paramValue = queryString;
				last = true;
			}

			char *key = core::string::getBeforeToken(&paramValue, "=", remainingBufSize(paramValue));
			char *value = paramValue;
			if (key == nullptr) {
				key = paramValue;
				static const char *EMPTY = "";
				value = (char*)EMPTY;
			}
			query.put(key, value);

			if (last) {
				break;
			}
		}
	}

	if (!parseHeaders(&bufPos)) {
		return;
	}

	content = bufPos;
	contentLength = remainingBufSize(bufPos);

	if (method == HttpMethod::GET) {
		_valid = contentLength == 0;
	} else if (method == HttpMethod::POST) {
		const char *value;
		if (!headers.get(header::CONTENT_LENGTH, value)) {
			_valid = false;
		} else {
			_valid = contentLength == SDL_atoi(value);
		}
	}
}

}
