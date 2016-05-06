/**
 * @file
 */

#include "ConnectDialog.h"
#include <QSettings>
#include "Settings.h"

#define S_HOSTNAME "hostname"
#define S_PORT "port"

ConnectDialog::ConnectDialog(const QString& defaultHostname, short defaultPort) :
		IDialog(tr("Connect to AI server")), _hostnameText(nullptr), _portText(nullptr), _group(
				nullptr) {
	Settings::setIfAbsent(S_HOSTNAME, defaultHostname);
	Settings::setIfAbsent(S_PORT, QString::number(defaultPort));

	const QSettings& settings = Settings::getSettings();
	_hostname = settings.value(S_HOSTNAME).toString();
	_port = settings.value(S_PORT).toInt();
}

ConnectDialog::~ConnectDialog() {
	delete _hostnameText;
	delete _portText;
	delete _group;
}

void ConnectDialog::addMainWidgets(QBoxLayout& layout) {
	_group = new QGroupBox(tr("Server"));
	QVBoxLayout *boxLayout = new QVBoxLayout;
	_hostnameText = new QLineEdit(_hostname);
	boxLayout->addWidget(_hostnameText);
	_portText = new QLineEdit(QString::number(_port));
	boxLayout->addWidget(_portText);
	_group->setLayout(boxLayout);
	layout.addWidget(_group);
}

void ConnectDialog::onApply() {
	_hostname = _hostnameText->text();
	_port = _portText->text().toShort();

	QSettings& settings = Settings::getSettings();
	settings.setValue(S_HOSTNAME, _hostname);
	settings.setValue(S_PORT, _portText->text());

	close();
}
