/**
 * @file
 */

#include "HttpStatus.h"

namespace http {

const char* toStatusString(HttpStatus status) {
	if (status == HttpStatus::InternalServerError) {
		return "Internal Server Error";
	} else if (status == HttpStatus::Ok) {
		return "OK";
	} else if (status == HttpStatus::NotFound) {
		return "Not Found";
	} else if (status == HttpStatus::NotImplemented) {
		return "Not Implemented";
	}
	return "Unknown";
}

}
