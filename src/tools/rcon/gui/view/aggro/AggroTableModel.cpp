/**
 * @file
 */
#include "AggroTableModel.h"
#include <QColor>

namespace ai {
namespace debug {

AggroTableModel::AggroTableModel(const AIDebugger& debugger, QTableView *tableView) :
		QAbstractTableModel(nullptr), _debugger(debugger), _parent(tableView) {
}

AggroTableModel::~AggroTableModel() {
}

void AggroTableModel::update() {
	beginResetModel();
	endResetModel();
}

int AggroTableModel::rowCount(const QModelIndex & /*parent*/) const {
	const core::DynamicArray<AIStateAggroEntry>& aggro = _debugger.getAggro();
	return aggro.size();
}

int AggroTableModel::columnCount(const QModelIndex & /*parent*/) const {
	return 2;
}

QVariant AggroTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation != Qt::Horizontal) {
		return QVariant();
	}

	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return tr("ID");
		case 1:
			return tr("Aggro");
		default:
			break;
		}
	}
	return QVariant();
}

QVariant AggroTableModel::data(const QModelIndex &mdlIndex, int role) const {
	const core::DynamicArray<AIStateAggroEntry>& aggro = _debugger.getAggro();
	if (role == Qt::DisplayRole) {
		switch (mdlIndex.column()) {
		case 0:
			return aggro[mdlIndex.row()].id;
		case 1:
			return aggro[mdlIndex.row()].aggro;
		default:
			break;
		}
	}
	return QVariant();
}

}
}
