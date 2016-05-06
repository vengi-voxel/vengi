/**
 * @file
 */

#pragma once

#include "AINodeStaticResolver.h"
#include <QAbstractItemModel>
#include <server/AIStubTypes.h>

namespace ai {
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

	inline BehaviourTreeModelItem* item(const QModelIndex& index) const {
		if (!index.isValid())
			return nullptr;

		return static_cast<BehaviourTreeModelItem*>(index.internalPointer());
	}
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	void setRootNode(AIStateNode* node);

public slots:
	bool submit() override;
	void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

signals:
	void behaviourUpdated();
};

}
}
