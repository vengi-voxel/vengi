/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "core/String.h"

namespace http {

class Url {
private:
	bool _valid = true;
	void parseSchema(char **strPtr);
	void parseHostPart(char **strPtr);
	void parsePath(char **strPtr);
	void parseQuery(char **strPtr);
	void parseFragment(char **strPtr);
public:
	explicit Url(const char *url);
	~Url();
	const std::string url;
	std::string schema;
	std::string username;
	std::string password;
	std::string hostname;
	uint16_t port = 80;
	std::string path;
	std::string query;
	std::string fragment;

	bool valid() const;
};

inline bool Url::valid() const {
	return _valid;
}

}