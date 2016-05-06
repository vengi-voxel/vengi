/**
 * @file
 */

#include "BehaviourTreeModel.h"
#include "BehaviourTreeModelItem.h"
#include "AIDebugger.h"

#include <QIcon>
#include <QDebug>

namespace ai {
namespace debug {

BehaviourTreeModel::BehaviourTreeModel(AIDebugger& debugger, AINodeStaticResolver& resolver, QObject *parent) :
		QAbstractItemModel(parent), _rootItem(nullptr), _resolver(resolver), _debugger(debugger), _allowUpdate(true) {
	connect(this, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
}

BehaviourTreeModel::~BehaviourTreeModel() {
	delete _rootItem;
}

void BehaviourTreeModel::onDataChanged(const QModelIndex& topLeft, const QModelIndex& /*bottomRight*/) {
	BehaviourTreeModelItem *nodeItem = item(topLeft);
	if (nodeItem == nullptr) {
		qDebug() << "No item found at: " << topLeft;
		return;
	}

	const QVariant& name = nodeItem->data(COL_NAME);
	const QVariant& type = nodeItem->data(COL_TYPE);
	const QVariant& condition = nodeItem->data(COL_CONDITION);
	_debugger.updateNode(nodeItem->node()->getNodeId(), name, type, condition);
	nodeItem->resetEdit();
}

QModelIndex BehaviourTreeModel::index(int row, int column, const QModelIndex &parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	BehaviourTreeModelItem *parentItem;
	if (!parent.isValid())
		parentItem = _rootItem;
	else
		parentItem = item(parent);

	BehaviourTreeModelItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex BehaviourTreeModel::parent(const QModelIndex &index) const {
	if (!index.isValid())
		return QModelIndex();

	BehaviourTreeModelItem *childItem = item(index);
	BehaviourTreeModelItem *parentItem = childItem->parent();
	if (parentItem == nullptr || parentItem == _rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int BehaviourTreeModel::rowCount(const QModelIndex &parent) const {
	BehaviourTreeModelItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = _rootItem;
	else
		parentItem = item(parent);

	if (parentItem == nullptr)
		return 0;

	return parentItem->childCount();
}

int BehaviourTreeModel::columnCount(const QModelIndex &parent) const {
	if (parent.isValid())
		return item(parent)->columnCount();
	return _rootItem->columnCount();
}

QVariant BehaviourTreeModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid())
		return QVariant();

	BehaviourTreeModelItem *nodeItem = item(index);
	if (nodeItem == nullptr)
		return QVariant();

	if (role == Qt::DecorationRole) {
		if (index.column() == COL_NAME)
			return nodeItem->icon();
	} else if (role == Qt::TextColorRole) {
		return nodeItem->color();
	}

	if (role == Qt::EditRole && _allowUpdate) {
		qDebug() << "start editing";
		_allowUpdate = false;
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		return nodeItem->data(index.column());
	else if (role == Qt::ToolTipRole)
		return nodeItem->tooltip(index.column());
	return QVariant();
}

bool BehaviourTreeModel::submit() {
	if (!_allowUpdate) {
		emit behaviourUpdated();
		qDebug() << "end editing";
	}
	_allowUpdate = true;
	return QAbstractItemModel::submit();
}

Qt::ItemFlags BehaviourTreeModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	switch (index.column()) {
	case COL_NAME:
	case COL_TYPE:
	case COL_CONDITION:
		flags |= Qt::ItemIsEditable;
	}
	return flags;
}

bool BehaviourTreeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	if (index.isValid() && role == Qt::EditRole) {
		_rootItem->child(index.row())->setData(index.column(), value);
		emit dataChanged(index, index);
		return true;
	}
	return false;
}

QVariant BehaviourTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && _rootItem != nullptr)
		return _rootItem->headerData(section);

	return QVariant();
}

void BehaviourTreeModel::setRootNode(AIStateNode* node) {
	if (!_allowUpdate)
		return;
	beginResetModel();
	if (_rootItem) {
		delete _rootItem;
		_rootItem = nullptr;
	}
	if (node->getNodeId() != -1) {
		_rootItem = new BehaviourTreeModelItem(node, _resolver);
	}
	endResetModel();
}

}
}
