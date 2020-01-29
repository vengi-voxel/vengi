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
inline core::String pwhash(const core::String& password, const core::String& salt) {
	// TODO:
	return password;
}

}
