#pragma once

#include <vector>
#include <string>

namespace core {

class Process {
public:
	/**
	 * @return 0 for success, anything else is an error
	 */
	static int exec (const std::string& command, const std::vector<std::string>& arguments);
};

}
