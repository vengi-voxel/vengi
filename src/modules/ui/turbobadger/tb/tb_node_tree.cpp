/**
 * @file
 */

#include "tb_node_tree.h"
#include "core/Assert.h"
#include "tb_language.h"
#include "tb_node_ref_tree.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"

namespace tb {

TBNode::~TBNode() {
	clear();
}

// static
TBNode *TBNode::create(const char *name) {
	TBNode *n = new TBNode;
	if ((n == nullptr) || ((n->m_name = SDL_strdup(name)) == nullptr)) {
		delete n;
		return nullptr;
	}
	return n;
}

// static
TBNode *TBNode::create(const char *name, int nameLen) {
	TBNode *n = new TBNode;
	if ((n == nullptr) || ((n->m_name = (char *)SDL_malloc(nameLen + 1)) == nullptr)) {
		delete n;
		return nullptr;
	}
	SDL_memcpy(n->m_name, name, nameLen);
	n->m_name[nameLen] = 0;
	return n;
}

// static
const char *TBNode::getNextNodeSeparator(const char *request) {
	while (*request != 0 && *request != '>') {
		request++;
	}
	return request;
}

TBNode *TBNode::getNode(const char *request, GET_MISS_POLICY mp) {
	// Iterate one node deeper for each sub request (non recursive)
	TBNode *n = this;
	while ((*request != 0) && (n != nullptr)) {
		const char *nextend = getNextNodeSeparator(request);
		int name_len = nextend - request;
		TBNode *n_child = n->getNodeInternal(request, name_len);
		if ((n_child == nullptr) && mp == GET_MISS_POLICY_CREATE) {
			n_child = n->create(request, name_len);
			if (n_child != nullptr) {
				n->add(n_child);
			}
		}
		n = n_child;
		request = *nextend == 0 ? nextend : nextend + 1;
	}
	return n;
}

TBNode *TBNode::getNodeFollowRef(const char *request, GET_MISS_POLICY mp) {
	TBNode *node = getNode(request, mp);
	if (node != nullptr) {
		node = TBNodeRefTree::followNodeRef(node);
	}
	return node;
}

TBNode *TBNode::getNodeInternal(const char *name, int nameLen) const {
	for (TBNode *n = getFirstChild(); n != nullptr; n = n->getNext()) {
		if (SDL_strncmp(n->m_name, name, nameLen) == 0 && n->m_name[nameLen] == 0) {
			return n;
		}
	}
	return nullptr;
}

bool TBNode::cloneChildren(TBNode *source, bool followRefs) {
	TBNode *item = source->getFirstChild();
	while (item != nullptr) {
		TBNode *new_child = create(item->m_name);
		if (new_child == nullptr) {
			return false;
		}

		new_child->m_value.copy(followRefs ? item->getValueFollowRef() : item->m_value);
		add(new_child);

		if (!new_child->cloneChildren(item, followRefs)) {
			return false;
		}
		item = item->getNext();
	}
	return true;
}

TBValue &TBNode::getValueFollowRef() {
	return TBNodeRefTree::followNodeRef(this)->getValue();
}

int TBNode::getValueInt(const char *request, int def) {
	TBNode *n = getNodeFollowRef(request);
	return n != nullptr ? n->m_value.getInt() : def;
}

float TBNode::getValueFloat(const char *request, float def) {
	TBNode *n = getNodeFollowRef(request);
	return n != nullptr ? n->m_value.getFloat() : def;
}

const char *TBNode::getValueString(const char *request, const char *def) {
	if (TBNode *node = getNodeFollowRef(request)) {
		// We might have a language string. Those are not
		// looked up in GetNode/ResolveNode.
		if (node->getValue().isString()) {
			const char *string = node->getValue().getString();
			if (*string == '@' && *TBNode::getNextNodeSeparator(string) == 0) {
				string = g_tb_lng->getString(string + 1);
			}
			return string;
		}
		return node->getValue().getString();
	}
	return def;
}

const char *TBNode::getValueStringRaw(const char *request, const char *def) {
	TBNode *n = getNodeFollowRef(request);
	return n != nullptr ? n->m_value.getString() : def;
}

class FileParser : public TBParserStream {
public:
	bool read(const char *filename, TBParserTarget *target) {
		f = TBFile::open(filename, TBFile::MODE_READ);
		if (f == nullptr) {
			return false;
		}
		TBParser p;
		TBParser::STATUS status = p.read(this, target);
		delete f;
		return status == TBParser::STATUS_OK;
	}
	virtual int getMoreData(char *buf, int bufLen) {
		return f->read(buf, 1, bufLen);
	}

private:
	TBFile *f;
};

class DataParser : public TBParserStream {
public:
	bool read(const char *data, int dataLen, TBParserTarget *target) {
		m_data = data;
		m_data_len = dataLen;
		TBParser p;
		TBParser::STATUS status = p.read(this, target);
		return status == TBParser::STATUS_OK;
	}
	virtual int getMoreData(char *buf, int bufLen) {
		const int consume = Min(bufLen, m_data_len);
		SDL_memcpy(buf, m_data, consume);
		m_data += consume;
		m_data_len -= consume;
		return consume;
	}

private:
	const char *m_data;
	int m_data_len;
};

class TBNodeTarget : public TBParserTarget {
public:
	TBNodeTarget(TBNode *root, const char *filename) {
		m_root_node = m_target_node = root;
		m_filename = filename;
	}
	virtual void onError(int lineNr, const char *error) {
#ifdef TB_RUNTIME_DEBUG_INFO
		Log::debug("%s(%d):Parse error: %s", m_filename, lineNr, error);
#endif // TB_RUNTIME_DEBUG_INFO
	}
	virtual void onComment(int lineNr, const char *comment) {
	}
	virtual void onToken(int lineNr, const char *name, TBValue &value) {
		if (m_target_node == nullptr) {
			return;
		}
		if (SDL_strcmp(name, "@file") == 0) {
			includeFile(lineNr, value.getString());
		} else if (SDL_strcmp(name, "@include") == 0) {
			includeRef(lineNr, value.getString());
		} else if (TBNode *n = TBNode::create(name)) {
			n->m_value.takeOver(value);
			m_target_node->add(n);
		}
	}
	virtual void enter() {
		if (m_target_node != nullptr) {
			m_target_node = m_target_node->getLastChild();
		}
	}
	virtual void leave() {
		core_assert(m_target_node != m_root_node);
		if (m_target_node != nullptr) {
			m_target_node = m_target_node->m_parent;
		}
	}
	void includeFile(int lineNr, const char *filename) {
		// Read the included file into a new TBNode and then
		// move all the children to m_target_node.
		TBTempBuffer include_filename;
		include_filename.appendPath(m_filename);
		include_filename.appendString(filename);
		TBNode content;
		if (content.readFile(include_filename.getData())) {
			while (TBNode *content_n = content.getFirstChild()) {
				content.remove(content_n);
				m_target_node->add(content_n);
			}
		} else {
			TBStr err;
			err.setFormatted("Referenced file \"%s\" was not found!", include_filename.getData());
			onError(lineNr, err);
		}
	}
	void includeRef(int lineNr, const char *refstr) {
		TBNode *refnode = nullptr;
		if (*refstr == '@') {
			TBNode tmp;
			tmp.getValue().setString(refstr, TBValue::SET_AS_STATIC);
			refnode = TBNodeRefTree::followNodeRef(&tmp);
		} else // Local look-up
		{
			// Note: If we read to a target node that already contains
			//       nodes, we might look up nodes that's already there
			//       instead of new nodes.
			refnode = m_root_node->getNode(refstr, TBNode::GET_MISS_POLICY_NULL);

			// Detect cycles
			TBNode *cycle_detection = m_target_node;
			while ((cycle_detection != nullptr) && (refnode != nullptr)) {
				if (cycle_detection == refnode) {
					refnode = nullptr; // We have a cycle, so just fail the inclusion.
				}
				cycle_detection = cycle_detection->getParent();
			}
		}
		if (refnode != nullptr) {
			m_target_node->cloneChildren(refnode);
		} else {
			TBStr err;
			err.setFormatted("Include \"%s\" was not found!", refstr);
			onError(lineNr, err);
		}
	}

private:
	TBNode *m_root_node;
	TBNode *m_target_node;
	const char *m_filename;
};

bool TBNode::readFile(const char *filename, TB_NODE_READ_FLAGS flags) {
	if ((flags & TB_NODE_READ_FLAGS_APPEND) == 0U) {
		clear();
	}
	FileParser p;
	TBNodeTarget t(this, filename);
	if (p.read(filename, &t)) {
		TBNodeRefTree::resolveConditions(this);
		return true;
	}
	return false;
}

bool TBNode::readData(const char *data, TB_NODE_READ_FLAGS flags) {
	return readData(data, SDL_strlen(data), flags);
}

bool TBNode::readData(const char *data, int dataLen, TB_NODE_READ_FLAGS flags) {
	if ((flags & TB_NODE_READ_FLAGS_APPEND) == 0U) {
		clear();
	}
	DataParser p;
	TBNodeTarget t(this, "{data}");
	if (!p.read(data, dataLen, &t)) {
		return false;
	}
	TBNodeRefTree::resolveConditions(this);
	return true;
}

void TBNode::clear() {
	SDL_free(m_name);
	m_name = nullptr;
	m_children.deleteAll();
}

} // namespace tb
