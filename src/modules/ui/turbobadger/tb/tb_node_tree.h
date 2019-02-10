/**
 * @file
 */

#pragma once

#include "parser/tb_parser.h"
#include "tb_linklist.h"

namespace tb {

enum TB_NODE_READ_FLAGS {
	TB_NODE_READ_FLAGS_NONE	= 0,
	/** Read nodes without clearing first. Can be used to append
		data from multiple sources, or inject dependencies. */
	TB_NODE_READ_FLAGS_APPEND = 1,
};
MAKE_ENUM_FLAG_COMBO(TB_NODE_READ_FLAGS);

/** TBNode is a tree node with a string name and a value (TBValue).
	It may have a parent TBNode and child TBNodes.

	Getting the value of this node or any child, may optionally follow
	references to nodes in any existing TBNodeRefTree (by name).

	During ReadFile/ReadData, it may also select which branches to include
	or exclude conditionally by lookup up values in TBNodeRefTree. */
class TBNode : public TBLinkOf<TBNode>
{
public:
	TBNode() : m_name(nullptr), m_parent(nullptr), m_cycle_id(0) {}
	~TBNode();

	/** Create a new node with the given name. */
	static TBNode *create(const char *name);

	/** Read a tree of nodes from file into this node. Returns true on success. */
	bool readFile(const char *filename, TB_NODE_READ_FLAGS flags = TB_NODE_READ_FLAGS_NONE);

	/** Read a tree of nodes from a null terminated string buffer. */
	bool readData(const char *data, TB_NODE_READ_FLAGS flags = TB_NODE_READ_FLAGS_NONE);

	/** Read a tree of nodes from a buffer with a known length. */
	bool readData(const char *data, int data_len, TB_NODE_READ_FLAGS flags = TB_NODE_READ_FLAGS_NONE);

	/** Clear the contens of this node. */
	void clear();

	/** Add node as child to this node. */
	void add(TBNode *n) { m_children.addLast(n); n->m_parent = this; }

	/** Add node before the reference node (which must be a child to this node). */
	void addBefore(TBNode *n, TBNode *reference) { m_children.addBefore(n, reference); n->m_parent = this; }

	/** Add node after the reference node (which must be a child to this node). */
	void addAfter(TBNode *n, TBNode *reference) { m_children.addAfter(n, reference); n->m_parent = this; }

	/** Remove child node n from this node. */
	void remove(TBNode *n) { m_children.remove(n); n->m_parent = nullptr; }

	/** Remove and delete child node n from this node. */
	void doDelete(TBNode *n) { m_children.doDelete(n); }

	/** Create duplicates of all items in source and add them to this node.
		If follow_refs is true, any references will be followed and the final target
		will be cloned instead of the ref node.
		Note: Nodes does not replace existing nodes with the same name. Cloned nodes
		are added after any existing nodes. */
	bool cloneChildren(TBNode *source, bool follow_refs = false);

	enum GET_MISS_POLICY {
		/** GetNode will return nullptr if the node doesn't exist. */
		GET_MISS_POLICY_NULL,
		/** GetNode will create all missing nodes for the request. */
		GET_MISS_POLICY_CREATE
	};

	/** Get a node from the given request.
		If the node doesn't exist, it will either return nullptr or create
		missing nodes, depending on the miss policy.
		It can find nodes in children as well. Names are separated by a ">".

		Ex: GetNode("dishes>pizza>special>batman") */
	TBNode *getNode(const char *request, GET_MISS_POLICY mp = GET_MISS_POLICY_NULL);

	/** Returns the name of this node. */
	const char *getName() const { return m_name; }

	/** Returns the value of this node. */
	TBValue &getValue() { return m_value; }

	/** Returns the value of this node.
		Will follow eventual references to TBNodeRefTree. */
	TBValue &getValueFollowRef();

	/** Get a value from the given request as an integer.
		Will follow eventual references to TBNodeRefTree.
		If the value is not specified, it returns the default value (def). */
	int getValueInt(const char *request, int def);

	/** Get a value from the given request as an float.
		Will follow eventual references to TBNodeRefTree.
		If the value is not specified, it returns the default value (def). */
	float getValueFloat(const char *request, float def);

	/** Get a value from the given request as an string.
		Will follow eventual references to TBNodeRefTree.
		Will also return any referenced language string.
		If the value is not specified, it returns the default value (def). */
	const char *getValueString(const char *request, const char *def);

	/** Same as getValueString, but won't look up language string references. */
	const char *getValueStringRaw(const char *request, const char *def);

	/** Get the next position in request that is a sub node separator,
		or the end of the string. */
	static const char *getNextNodeSeparator(const char *request);

	inline TBNode *getParent() const { return m_parent; }
	inline TBNode *getFirstChild() const { return m_children.getFirst(); }
	inline TBNode *getLastChild() const { return m_children.getLast(); }
private:
friend class TBNodeTarget;
friend class TBNodeRefTree;
	TBNode *getNodeFollowRef(const char *request,
							GET_MISS_POLICY mp = GET_MISS_POLICY_NULL);
	TBNode *getNodeInternal(const char *name, int name_len) const;
	static TBNode *create(const char *name, int name_len);
	char *m_name;
	TBValue m_value;
	TBLinkListOf<TBNode> m_children;
	TBNode *m_parent;
	uint32_t m_cycle_id;	///< Used to detect circular references.
};

} // namespace tb
