/**
 * @file
 */
#pragma once

#include <QAbstractTableModel>
#include <QTableView>
#include "ai/SimpleAI.h"
#include "ai/server/AIStubTypes.h"

namespace ai {
namespace debug {

class AIDebugger;

class EntityListModel: public QAbstractTableModel {
Q_OBJECT
private:
	AIDebugger& _debugger;
	QTableView* _parent;
	QList<AIStateWorld> _list;
public:
	EntityListModel(AIDebugger& debugger, QTableView *tableView);
	~EntityListModel();

	inline const QList<AIStateWorld>& getEntities() const {
		return _list;
	}

	void update();
	QModelIndex characterIndex(CharacterId id) const;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
			int role) const override;
};

}
}
