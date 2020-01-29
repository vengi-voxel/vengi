/**
 * @file
 */
#pragma once

#include <QApplication>
#include <vector>
#include <string>

class CompressorProxy: public QObject {
Q_OBJECT
private:
	inline bool emitCheck(bool & flag) {
		flag = true;
		QCoreApplication::sendPostedEvents(this, QEvent::MetaCall); // recurse
		bool result = flag;
		flag = false;
		return result;
	}

	bool _selected;
	bool _entitiesUpdated;
	bool _namesReceived;

private slots:
	void selected() {
		if (!emitCheck(_selected)) {
			return;
		}
		emit onSelected();
	}

	void entitiesUpdated() {
		if (!emitCheck(_entitiesUpdated)) {
			return;
		}
		emit onEntitiesUpdated();
	}

	void namesReceived() {
		if (!emitCheck(_namesReceived)) {
			return;
		}
		emit onNamesReceived();
	}

signals:
	// signal that is triggered whenever the entity details for the current selected entity arrived
	void onSelected();
	// new names list was received
	void onNamesReceived();
	// entities on the map were updated
	void onEntitiesUpdated();

public:
	// No default constructor, since the proxy must be a child of the
	// target object.
	explicit CompressorProxy(QObject * parentObj) :
			QObject(parentObj), _selected(false), _entitiesUpdated(false), _namesReceived(false) {
	}
};
