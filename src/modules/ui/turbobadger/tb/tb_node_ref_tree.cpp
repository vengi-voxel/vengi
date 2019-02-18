/**
 * @file
 */

#include "tb_node_ref_tree.h"
#include "tb_language.h"
#include "tb_system.h"

namespace tb {

// static
TBLinkListOf<TBNodeRefTree> TBNodeRefTree::s_ref_trees;

TBNodeRefTree::TBNodeRefTree(const char *name) : m_name(name), m_name_id(name) {
	s_ref_trees.addLast(this);
}

TBNodeRefTree::~TBNodeRefTree() {
	s_ref_trees.remove(this);
}

TBValue &TBNodeRefTree::getValue(const char *request) {
	if (TBNode *node = m_node.getNodeFollowRef(request))
		return node->getValue();
	Log::debug("TBNodeRefTree::getValue - Request not found: %s", request);
	static TBValue nullval;
	return nullval;
}

// static
TBValue &TBNodeRefTree::getValueFromTree(const char *request) {
	core_assert(*request == '@');
	TBNode tmp;
	tmp.getValue().setString(request, TBValue::SET_AS_STATIC);
	TBNode *node = TBNodeRefTree::followNodeRef(&tmp);
	if (node != &tmp)
		return node->getValue();
	static TBValue nullval;
	return nullval;
}

void TBNodeRefTree::setValue(const char *request, const TBValue &value) {
	if (TBNode *node = m_node.getNode(request, TBNode::GET_MISS_POLICY_CREATE)) {
		// FIX: Only invoke the listener if it really changed.
		node->getValue().copy(value);
		invokeChangeListenersInternal(request);
	}
}

void TBNodeRefTree::invokeChangeListenersInternal(const char *request) {
	TBLinkListOf<TBNodeRefTreeListener>::Iterator iter = m_listeners.iterateForward();
	while (TBNodeRefTreeListener *listener = iter.getAndStep())
		listener->onDataChanged(this, request);
}

// static
TBNodeRefTree *TBNodeRefTree::getRefTree(const char *name, int nameLen) {
	for (TBNodeRefTree *rt = s_ref_trees.getFirst(); rt; rt = rt->getNext())
		if (strncmp(rt->getName(), name, nameLen) == 0)
			return rt;
	return nullptr;
}

// static
TBNode *TBNodeRefTree::followNodeRef(TBNode *node) {
	// Detect circular loops by letting this call get a unique id.
	// Update the id on each visited node and if it's already set,
	// there's a loop. This cost the storage of id in each TBNode,
	// and assumes the look up doesn't cause other lookups
	// recursively.
	// FIX: Switch to hare and teleporting tortouise?
	static uint32_t s_cycle_id = 0;
	uint32_t cycle_id = ++s_cycle_id;
	TBNode *start_node = node;

	while (node->getValue().isString()) {
		// If not a reference at all, we're done.
		const char *node_str = node->getValue().getString();
		if (*node_str != '@')
			break;

		// If there's no tree name and request, we're done. It's probably a language string.
		const char *name_start = node_str + 1;
		const char *name_end = TBNode::getNextNodeSeparator(name_start);
		if (*name_end == 0)
			break;

		TBNode *next_node = nullptr;

		// We have a "@>noderequest" string. Go ahead and do a local look up.
		if (*name_start == '>') {
			TBNode *local_root = node;
			while (local_root->getParent())
				local_root = local_root->getParent();
			next_node = local_root->getNode(name_start + 1, TBNode::GET_MISS_POLICY_NULL);
		}
		// We have a "@treename>noderequest" string. Go ahead and look it up from the right node tree.
		else if (TBNodeRefTree *rt = TBNodeRefTree::getRefTree(name_start, name_end - name_start)) {
			next_node = rt->m_node.getNode(name_end + 1, TBNode::GET_MISS_POLICY_NULL);
		} else {
			Log::debug("TBNodeRefTree::ResolveNode - No tree found for request \"%s\" from node \"%s\"", node_str,
					   node->getValue().getString());
			break;
		}

		if (!next_node) {
			Log::debug("TBNodeRefTree::ResolveNode - Node not found on request \"%s\"", node_str);
			break;
		}
		node = next_node;

		// Detect circular reference loop.
		if (node->m_cycle_id != cycle_id)
			node->m_cycle_id = cycle_id;
		else {
			Log::debug("TBNodeRefTree::ResolveNode - Reference loop detected on request \"%s\" from node \"%s\"",
					   node_str, node->getValue().getString());
			return start_node;
		}
	}
	return node;
}

// static
void TBNodeRefTree::resolveConditions(TBNode *parentNode) {
	bool condition_ret = false;
	TBNode *node = parentNode->getFirstChild();
	while (node) {
		bool delete_node = false;
		bool move_children = false;
		if (strcmp(node->getName(), "@if") == 0) {
			condition_ret = node->getValueFollowRef().getInt() ? true : false;
			if (condition_ret)
				move_children = true;
			delete_node = true;
		} else if (strcmp(node->getName(), "@else") == 0) {
			condition_ret = !condition_ret;
			if (condition_ret)
				move_children = true;
			delete_node = true;
		}

		// Make sure we'll skip any nodes added from a conditional branch.
		TBNode *node_next = node->getNext();

		if (move_children) {
			// Resolve the branch first, since we'll skip it.
			resolveConditions(node);
			while (TBNode *content = node->getLastChild()) {
				node->remove(content);
				parentNode->addAfter(content, node);
			}
		}

		if (delete_node)
			parentNode->doDelete(node);
		else
			resolveConditions(node);
		node = node_next;
	}
}

} // namespace tb
