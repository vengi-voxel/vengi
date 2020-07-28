/**
 * @file
 */

#include "tb_language.h"
#include "tb_node_tree.h"
#include "tb_system.h"

namespace tb {

TBLanguage::~TBLanguage() {
	clear();
}

bool TBLanguage::load(const char *filename) {
	// Read the file into a node tree (even though it's only a flat list)
	TBNode node;
	if (!node.readFile(filename)) {
		return false;
	}

	// Go through all nodes and add to the strings hash table
	TBNode *n = node.getFirstChild();
	while (n != nullptr) {
		const char *str = n->getValue().getString();
		core::String *new_str = new core::String(str);
		if ((new_str == nullptr) || !strings.add(TBID(n->getName()), new_str)) {
			delete new_str;
			return false;
		}
		n = n->getNext();
	}
	return true;
}

void TBLanguage::clear() {
	strings.deleteAll();
}

const char *TBLanguage::getString(const TBID &id) {
	if (core::String *str = strings.get(id)) {
		return str->c_str();
	}
	return "<TRANSLATE!>";
}

} // namespace tb
