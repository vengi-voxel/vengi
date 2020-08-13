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

BehaviourTreeModel::BehaviourTreeModel(AIDebugger& debugger, AINodeStaticResolver& resolver, QObject *objParent) :
		QAbstractItemModel(objParent), _rootItem(nullptr), _resolver(resolver), _debugger(debugger), _allowUpdate(true) {
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

QModelIndex BehaviourTreeModel::index(int row, int column, const QModelIndex &parentIndex) const {
	if (!hasIndex(row, column, parentIndex)) {
		return QModelIndex();
	}

	BehaviourTreeModelItem *parentItem;
	if (!parentIndex.isValid()) {
		parentItem = _rootItem;
	} else {
		parentItem = item(parentIndex);
	}

	BehaviourTreeModelItem *childItem = parentItem->child(row);
	if (childItem != nullptr) {
		return createIndex(row, column, childItem);
	}
	return QModelIndex();
}

QModelIndex BehaviourTreeModel::parent(const QModelIndex &mdlIndex) const {
	if (!mdlIndex.isValid()) {
		return QModelIndex();
	}

	BehaviourTreeModelItem *childItem = item(mdlIndex);
	BehaviourTreeModelItem *parentItem = childItem->parent();
	if (parentItem == nullptr || parentItem == _rootItem) {
		return QModelIndex();
	}

	return createIndex(parentItem->row(), 0, parentItem);
}

int BehaviourTreeModel::rowCount(const QModelIndex &parentIndex) const {
	BehaviourTreeModelItem *parentItem;
	if (parentIndex.column() > 0) {
		return 0;
	}

	if (!parentIndex.isValid()) {
		parentItem = _rootItem;
	} else {
		parentItem = item(parentIndex);
	}

	if (parentItem == nullptr) {
		return 0;
	}

	return parentItem->childCount();
}

int BehaviourTreeModel::columnCount(const QModelIndex &parentIndex) const {
	if (parentIndex.isValid()) {
		return item(parentIndex)->columnCount();
	}
	if (_rootItem == nullptr) {
		return 0;
	}
	return _rootItem->columnCount();
}

QVariant BehaviourTreeModel::data(const QModelIndex &mdlIndex, int role) const {
	if (!mdlIndex.isValid()) {
		return QVariant();
	}

	BehaviourTreeModelItem *nodeItem = item(mdlIndex);
	if (nodeItem == nullptr) {
		return QVariant();
	}

	if (role == Qt::DecorationRole) {
		if (mdlIndex.column() == COL_NAME) {
			return nodeItem->icon();
		}
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
	} else if (role == Qt::ForegroundRole) {
#else
	} else if (role == Qt::TextColorRole) {
#endif
		return nodeItem->color();
	}

	if (role == Qt::EditRole && _allowUpdate) {
		qDebug() << "start editing";
		_allowUpdate = false;
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		return nodeItem->data(mdlIndex.column());
	}
	if (role == Qt::ToolTipRole) {
		return nodeItem->tooltip(mdlIndex.column());
	}
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

Qt::ItemFlags BehaviourTreeModel::flags(const QModelIndex &mdlIndex) const {
	if (!mdlIndex.isValid()) {
		return Qt::ItemIsEnabled;
	}

	Qt::ItemFlags itemflags = QAbstractItemModel::flags(mdlIndex);
	switch (mdlIndex.column()) {
	case COL_NAME:
	case COL_TYPE:
	case COL_CONDITION:
		itemflags |= Qt::ItemIsEditable;
	}
	return itemflags;
}

bool BehaviourTreeModel::setData(const QModelIndex &mdlIndex, const QVariant &value, int role) {
	if (mdlIndex.isValid() && role == Qt::EditRole) {
		_rootItem->child(mdlIndex.row())->setData(mdlIndex.column(), value);
		emit dataChanged(mdlIndex, mdlIndex);
		return true;
	}
	return false;
}

QVariant BehaviourTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && _rootItem != nullptr) {
		return _rootItem->headerData(section);
	}

	return QVariant();
}

bool BehaviourTreeModel::setRootNode(AIStateNode* node) {
	if (!_allowUpdate) {
		return false;
	}
	beginResetModel();
	if (_rootItem != nullptr) {
		delete _rootItem;
		_rootItem = nullptr;
	}
	if (node->getNodeId() != -1) {
		_rootItem = new BehaviourTreeModelItem(node, _resolver);
	}
	endResetModel();
	return true;
}

}
}
