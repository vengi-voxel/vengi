/**
 * @file
 */
#include "StateTableModel.h"
#include "core/Trace.h"
#include <QColor>

namespace ai {
namespace debug {

StateTableModel::StateTableModel(const AIDebugger& debugger) :
		QAbstractTableModel(nullptr), _debugger(debugger) {
}

StateTableModel::~StateTableModel() {
}

void StateTableModel::update() {
	core_trace_scoped(StateTableModelUpdate);
	beginResetModel();
	const AIDebugger::CharacterAttributesMap& a = _debugger.getAttributes();
	_list.clear();
	for (AIDebugger::CharacterAttributesMap::const_iterator i = a.begin(); i != a.end(); ++i) {
		_list << i.key();
	}
	endResetModel();
}

int StateTableModel::rowCount(const QModelIndex & /*parent*/) const {
	return _list.size();
}

int StateTableModel::columnCount(const QModelIndex & /*parent*/) const {
	return 2;
}

QVariant StateTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation != Qt::Horizontal) {
		return QVariant();
	}

	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return tr("Key");
		case 1:
			return tr("Value");
		default:
			break;
		}
	}
	return QVariant();
}

QVariant StateTableModel::data(const QModelIndex &mdlIndex, int role) const {
	const QString& key = _list[mdlIndex.row()];
	if (role == Qt::DisplayRole) {
		switch (mdlIndex.column()) {
		case 0:
			return key;
		case 1:
			return _debugger.getAttributes().value(key);
		default:
			break;
		}
	}
	return QVariant();
}

}
}
