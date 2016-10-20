/**
 * @file
 */

#pragma once

namespace core {

/**
 * @brief Non-blocking console input reading e.g. for dedicated server command line
 */
class Input {
private:
	char _input[256];
public:
	Input();

	const char* read();
};

}
