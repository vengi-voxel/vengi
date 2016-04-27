#pragma once

#include <string>

namespace ai {

class LUATreeLoader;
class LUANode;
class IAIFactory;

class LUATree {
private:
	std::string _name;
	LUATreeLoader* _ctx;
	LUANode* _root;
public:
	LUATree(const std::string& name, LUATreeLoader* ctx) :
			_name(name), _ctx(ctx), _root(nullptr) {
	}

	const IAIFactory& getAIFactory() const;

	bool setRoot(LUANode* root);

	inline const std::string& getName() const {
		return _name;
	}

	inline LUANode* getRoot() const {
		return _root;
	}
};

}
