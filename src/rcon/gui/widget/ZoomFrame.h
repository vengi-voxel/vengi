/**
 * @file
 */

#pragma once

#include <QFrame>
#include <QGraphicsView>

namespace ai {
namespace debug {

class ZoomWidget;

class ZoomFrame: public QFrame {
	Q_OBJECT
protected:
	ZoomWidget* _zoomWidget;
	QGraphicsView* _graphicsView;
public:
	ZoomFrame(QGraphicsView* graphicsView, QWidget* parent = nullptr);
	virtual ~ZoomFrame();

	void zoomIn(int level);
	void zoomOut(int level);
};

}
}
