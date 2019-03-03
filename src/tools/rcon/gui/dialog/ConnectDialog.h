/**
 * @file
 */
#pragma once

#include "IDialog.h"

#include <QLineEdit>
#include <QGroupBox>

// TODO: implement history of last connects
class ConnectDialog: public IDialog {
Q_OBJECT
private:
	QLineEdit *_hostnameText;
	QLineEdit *_portText;
	QGroupBox *_group;
	QString _hostname;
	short _port;
public:
	ConnectDialog(const QString& defaultHostname = "0.0.0.0", short defaultPort = 12345);
	virtual ~ConnectDialog();

	virtual void onApply() override;
	virtual void addMainWidgets(QBoxLayout& boxLayout) override;

	inline const QString& getHostname() const {
		return _hostname;
	}

	inline short getPort() const {
		return _port;
	}
};
