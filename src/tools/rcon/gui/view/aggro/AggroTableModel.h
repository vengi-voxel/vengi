/**
 * @file
 */
#pragma once

#include <QAbstractTableModel>
#include <QTableView>

#include "AIDebugger.h"

namespace ai {
namespace debug {

class AggroTableModel: public QAbstractTableModel {
Q_OBJECT
private:
	const AIDebugger& _debugger;
	QTableView* _parent;
public:
	AggroTableModel(const AIDebugger& debugger, QTableView *tableView);
	~AggroTableModel();

	inline const AIDebugger::Entities& getEntities() const {
		return _debugger.getEntities();
	}

	inline const AIStateWorld* getEntity(const QModelIndex &mdlIndex) const {
		const int size = getEntities().size();
		if (size > mdlIndex.row() && mdlIndex.row() >= 0) {
			return &getEntities().values().at(mdlIndex.row());
		}
		return nullptr;
	}

	void update();

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
			int role) const override;
};

}
}
