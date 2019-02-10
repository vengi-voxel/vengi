/**
 * @file
 */
#pragma once

#include "AINodeStaticResolver.h"
#include <QAbstractItemModel>

namespace ai {

class AIStateNode;

namespace debug {

class AIDebugger;
class BehaviourTreeModelItem;

class BehaviourTreeModel: public QAbstractItemModel {
Q_OBJECT
private:
	BehaviourTreeModelItem *_rootItem;
	AINodeStaticResolver& _resolver;
	AIDebugger& _debugger;
	mutable bool _allowUpdate;
public:
	explicit BehaviourTreeModel(AIDebugger& debugger, AINodeStaticResolver& resolver, QObject *parent = nullptr);
	~BehaviourTreeModel();

	inline BehaviourTreeModelItem* item(const QModelIndex& mdlIndex) const {
		if (!mdlIndex.isValid())
			return nullptr;

		return static_cast<BehaviourTreeModelItem*>(mdlIndex.internalPointer());
	}
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &mdlIndex) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	bool setRootNode(AIStateNode* node);

	inline bool editMode() const {
		return !_allowUpdate;
	}

	inline void abortEditMode() {
		_allowUpdate = true;
	}

public slots:
	bool submit() override;
	void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

signals:
	void behaviourUpdated();
};

}
}
