/**
 * @file
 */

#pragma once

#include <QGraphicsView>

namespace ai {
namespace debug {

class IGraphicsView : public QGraphicsView {
private:
	bool _renderGrid;
	bool _renderBackground;
public:
	IGraphicsView(bool renderGrid = false, bool renderBackground = false, QWidget* parent = nullptr);
	virtual ~IGraphicsView();

	virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
};

}
}
