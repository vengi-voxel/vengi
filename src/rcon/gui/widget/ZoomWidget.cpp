#include "ZoomWidget.h"

#include <QStyle>
#include <qmath.h>

namespace ai {
namespace debug {

ZoomWidget::ZoomWidget(QGraphicsView& gview, QWidget* parent) :
		QWidget(parent), _graphicsView(gview), _zoomSlider(), _zoomInButton(), _zoomOutButton() {
	const int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
	QSize icosize(size, size);

	_zoomInButton.setAutoRepeat(true);
	_zoomInButton.setAutoRepeatInterval(33);
	_zoomInButton.setAutoRepeatDelay(0);
	_zoomInButton.setIcon(QPixmap(":/images/zoomin.png"));
	_zoomInButton.setIconSize(icosize);

	_zoomOutButton.setAutoRepeat(true);
	_zoomOutButton.setAutoRepeatInterval(33);
	_zoomOutButton.setAutoRepeatDelay(0);
	_zoomOutButton.setIcon(QPixmap(":/images/zoomout.png"));
	_zoomOutButton.setIconSize(icosize);

	_zoomSlider.setMinimum(0);
	_zoomSlider.setMaximum(200);
	_zoomSlider.setValue(100);
	_zoomSlider.setTickPosition(QSlider::TicksRight);

	setupZoomMatrix();

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(&_zoomInButton);
	layout->addWidget(&_zoomSlider);
	layout->addWidget(&_zoomOutButton);

	connect(&_zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setupZoomMatrix()));
	connect(&_zoomInButton, SIGNAL(clicked()), this, SLOT(zoomIn()));
	connect(&_zoomOutButton, SIGNAL(clicked()), this, SLOT(zoomOut()));
	setLayout(layout);
}

ZoomWidget::~ZoomWidget() {
	disconnect(&_zoomSlider, 0, this, SLOT(setupZoomMatrix()));
	disconnect(&_zoomInButton, 0, this, SLOT(zoomIn()));
	disconnect(&_zoomOutButton, 0, this, SLOT(zoomOut()));
}

void ZoomWidget::setupZoomMatrix() {
	const qreal scale = _zoomSlider.value() / 100.0;
	QMatrix matrix;
	matrix.scale(scale, scale);
	_graphicsView.setMatrix(matrix);
}

void ZoomWidget::setAutoRepeatInterval(int interval) {
	_zoomInButton.setAutoRepeatInterval(interval);
	_zoomOutButton.setAutoRepeatInterval(interval);
}

void ZoomWidget::setValueRange(int min, int max) {
	_zoomSlider.setMinimum(min);
	_zoomSlider.setMaximum(max);
}

void ZoomWidget::setValue(int value) {
	_zoomSlider.setValue(value);
}

void ZoomWidget::zoomIn(int level) {
	_zoomSlider.setValue(_zoomSlider.value() + level);
}

void ZoomWidget::zoomOut(int level) {
	_zoomSlider.setValue(_zoomSlider.value() - level);
}

}
}
