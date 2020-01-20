/**
 * @file
 */
#pragma once

#include <memory>

namespace core {

template<class TYPE>
class Factory {
public:
	template<class ...ARGS>
	std::shared_ptr<TYPE> create (ARGS... args) const {
		return std::make_shared<TYPE>(std::forward<ARGS>(args)...);
	}
};

}
