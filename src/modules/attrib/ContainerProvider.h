#pragma once

#include "Container.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace attrib {

class ContainerProvider {
private:
	typedef std::unordered_map<std::string, ContainerPtr> Containers;
	Containers _containers;
	std::string _error;
public:
	ContainerProvider();

	bool init();

	void addContainer(const ContainerPtr& container);
	ContainerPtr getContainer(const std::string& name) const;

	inline const std::string& error() const {
		return _error;
	}
};

typedef std::shared_ptr<ContainerProvider> ContainerProviderPtr;

}
