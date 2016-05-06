/**
 * @file
 */

#include "ZoomFrame.h"
#include "MapView.h"
#include "ZoomWidget.h"
#include "AIDebugger.h"

namespace ai {
namespace debug {

ZoomFrame::ZoomFrame(QGraphicsView* graphicsView, QWidget* parent) :
		QFrame(parent), _zoomWidget(nullptr), _graphicsView(graphicsView) {
	setFrameStyle(Sunken | StyledPanel);
	_graphicsView->setRenderHint(QPainter::Antialiasing, false);
	_graphicsView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
	//_graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	_graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	_graphicsView->setCacheMode(QGraphicsView::CacheBackground);
	_zoomWidget = new ZoomWidget(*_graphicsView);

	setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

	QHBoxLayout* topLayout = new QHBoxLayout;
	topLayout->addWidget(_graphicsView);
	topLayout->addWidget(_zoomWidget);
	setLayout(topLayout);
}

ZoomFrame::~ZoomFrame() {
	delete _zoomWidget;
}

void ZoomFrame::zoomIn(int level) {
	_zoomWidget->zoomIn(level);
}

void ZoomFrame::zoomOut(int level) {
	_zoomWidget->zoomOut(level);
}

}
}
