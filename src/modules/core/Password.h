/**
 * @file
 */

#pragma once

#include "core/Hash.h"
#include "core/String.h"

namespace core {

/**
 * @brief Apply a salt on the given password to never send the clear text password
 * to the server. The password should only be kept on the client side as it might
 * be used for other stuff, too.
 */
inline std::string pwhash(const std::string& password, const std::string& salt) {
	// TODO:
	return password;
}

}
