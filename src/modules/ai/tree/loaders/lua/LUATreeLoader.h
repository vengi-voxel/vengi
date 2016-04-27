#pragma once

#include "tree/loaders/ITreeLoader.h"

namespace ai {

/**
 * @brief Implementation of @c ITreeLoader that gets its data from a lua script
 */
class LUATreeLoader: public ai::ITreeLoader {
public:
	LUATreeLoader(const IAIFactory& aiFactory);
	/**
	 * @brief this will initialize the loader once with all the defined behaviours from the given lua string.
	 */
	bool init(const std::string& luaString);
};

}
