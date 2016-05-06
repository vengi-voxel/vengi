/**
 * @file
 */

#include "EntityListModel.h"
#include <QColor>

namespace ai {
namespace debug {

EntityListModel::EntityListModel(AIDebugger& debugger, QTableView *parent) :
		QAbstractTableModel(parent), _debugger(debugger), _parent(parent) {
}

EntityListModel::~EntityListModel() {
}

QModelIndex EntityListModel::characterIndex(CharacterId id) const {
	int row = 0;
	for (const AIStateWorld& state : _list) {
		if (state.getId() == id)
			return createIndex(row, 0);
		++row;
	}
	qDebug() << "Could not find entity " << id << " in the model";
	return QModelIndex();
}

void EntityListModel::update() {
	beginResetModel();
	_list = _debugger.getEntities().values();
	// TODO: sort list - not model
	endResetModel();
}

int EntityListModel::rowCount(const QModelIndex & /*parent*/) const {
	return _list.size();
}

int EntityListModel::columnCount(const QModelIndex & /*parent*/) const {
	return 1;
}

QVariant EntityListModel::headerData(int section, Qt::Orientation orientation,
		int role) const {
	if (orientation != Qt::Horizontal)
		return QVariant();
	if (section != 0)
		return QVariant();

	if (role == Qt::DisplayRole) {
		return tr("Entities");
	} else if (role == Qt::ToolTipRole) {
		return tr("The character id");
	}
	return QVariant();
}

QVariant EntityListModel::data(const QModelIndex &index, int role) const {
	const AIStateWorld& state = _list.at(index.row());
	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			const CharacterAttributes& attributes = state.getAttributes();
			auto name = attributes.find(attributes::NAME);
			if (name != attributes.end()) {
				return QString::fromStdString(name->second).append(" (%1)").arg(state.getId());
			}
			return state.getId();
		}
	} else if (role == Qt::BackgroundColorRole) {
		if (_debugger.isSelected(state)) {
			return QColor(Qt::gray);
		}
	}
	return QVariant();
}

}
}
