/**
 * @file
 */
#pragma once

#include <QVariant>
#include <QIcon>
#include <SimpleAI.h>

namespace ai {
namespace debug {

enum {
	COL_NAME,
	COL_TYPE,
	COL_CONDITION,
	COL_STATE,
	COL_LASTRUN,

	COL_MAX
};

class AINodeStaticResolver;

class BehaviourTreeModelItem {
private:
	AIStateNode* _node;
	const AIStateNodeStatic& _staticNodeData;
	QList<BehaviourTreeModelItem*> _rows;
	BehaviourTreeModelItem* _parent;
	QIcon _icon;

	QString _editedType;
	QString _editedName;
	QString _editedCondition;

public:
	BehaviourTreeModelItem(AIStateNode* stateNodePtr, AINodeStaticResolver& resolver, BehaviourTreeModelItem* modelItem = nullptr);
	virtual ~BehaviourTreeModelItem();

	void setData(int column, const QVariant& data);

	void resetEdit();

	QVariant headerData(int column) const;
	QVariant data(int column) const;
	QString tooltip(int column) const;
	QIcon icon() const;

	QVariant color() const;

	inline int columnCount() const {
		return COL_MAX;
	}

	inline int childCount() const {
		return _rows.size();
	}

	int row();

	BehaviourTreeModelItem* child(int row);

	inline BehaviourTreeModelItem* parent() {
		return _parent;
	}

	inline AIStateNode* node() {
		return _node;
	}
};

}
}
