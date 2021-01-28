/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(SignupValidationState) {
	client->validationState(message->state());
}
