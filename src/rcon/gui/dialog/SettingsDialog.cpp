/**
 * @file
 */
#include "SettingsDialog.h"
#include "Settings.h"
#include <QColorDialog>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QIntValidator>
#include <QLineEdit>

namespace ai {
namespace debug {

SettingsDialog::SettingsDialog() :
		IDialog(tr("Settings"), DIALOG_NO_APPLY_BUTTON) {
}

QGroupBox* SettingsDialog::createMapView() {
	QGroupBox* mapView = new QGroupBox(tr("Map view"), this);
	QGridLayout* layout = new QGridLayout(this);

	// TODO: find something better for the colors
	QColorDialog *bgColor = new QColorDialog(this);
	bgColor->setWindowFlags(Qt::SubWindow);
	bgColor->setOptions(QColorDialog::DontUseNativeDialog | QColorDialog::NoButtons);
	bgColor->setCurrentColor(Settings::getBackgroundColor());
	connect(bgColor, &QColorDialog::currentColorChanged, this, Settings::setBackgroundColor);

	QCheckBox* showGrid = new QCheckBox(this);
	showGrid->setChecked(Settings::getGrid());
	connect(showGrid, &QCheckBox::stateChanged, this, &SettingsDialog::setShowGrid);

	QLineEdit* gridInterval = new QLineEdit(this);
	gridInterval->setText(QString::number(Settings::getGridInterval()));
	gridInterval->setValidator(new QIntValidator(0, 10000, gridInterval));
	connect(gridInterval, &QLineEdit::textChanged, this, &SettingsDialog::setGridInterval);

	QLineEdit* itemSize = new QLineEdit(this);
	itemSize->setText(QString::number(Settings::getItemSize()));
	itemSize->setValidator(new QDoubleValidator(1.0, 100.0, 1, itemSize));
	connect(itemSize, &QLineEdit::textChanged, this, &SettingsDialog::setItemSize);

	int row = 0;
	layout->addWidget(new QLabel(tr("Show grid"), this), row, 0, Qt::AlignTop);
	layout->addWidget(showGrid, row, 1);
	++row;

	layout->addWidget(new QLabel(tr("Grid interval"), this), row, 0, Qt::AlignTop);
	layout->addWidget(gridInterval, row, 1);
	++row;

	layout->addWidget(new QLabel(tr("Item size"), this), row, 0, Qt::AlignTop);
	layout->addWidget(itemSize, row, 1);
	++row;

	layout->addWidget(new QLabel(tr("Background"), this), row, 0, Qt::AlignTop);
	layout->addWidget(bgColor, row, 1);
	++row;

	mapView->setLayout(layout);
	return mapView;
}

void SettingsDialog::setGridInterval(const QString& value) {
	Settings::setGridInterval(value.toInt());
}

void SettingsDialog::setItemSize(const QString& value) {
	Settings::setItemSize(value.toFloat());
}

void SettingsDialog::setShowGrid(int value) {
	Settings::setGrid(value != 0);
}

void SettingsDialog::addMainWidgets(QBoxLayout& layout) {
	QSettings settings;

	QGroupBox* mapView = createMapView();
	layout.addWidget(mapView);
}

}
}
