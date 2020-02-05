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
	explicit Url(const core::String& url);
	~Url();
	const core::String url;
	core::String schema;
	core::String username;
	core::String password;
	core::String hostname;
	uint16_t port = 80;
	core::String path;
	core::String query;
	core::String fragment;

	bool valid() const;
};

inline bool Url::valid() const {
	return _valid;
}

}