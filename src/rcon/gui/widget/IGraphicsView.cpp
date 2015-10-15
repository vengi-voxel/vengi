#include "IGraphicsView.h"

namespace ai {
namespace debug {

IGraphicsView::IGraphicsView(bool renderGrid, bool renderBackground, QWidget* parent) :
		QGraphicsView(parent), _renderGrid(renderGrid), _renderBackground(renderBackground) {
	setRenderHint(QPainter::Antialiasing, true);
}

IGraphicsView::~IGraphicsView() {
}

void IGraphicsView::drawBackground(QPainter* painter, const QRectF& rect) {
	QGraphicsView::drawBackground(painter, rect);
	if (_renderBackground) {
		painter->fillRect(rect, QBrush(QColor(50, 50, 50)));
	}

	if (_renderGrid) {
		QPen linePen(QColor(80, 80, 80), 1, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
		linePen.setCosmetic(true);
		painter->setPen(linePen);

		const int gridInterval = 100;
		const qreal left = static_cast<int>(rect.left())
				- (static_cast<int>(rect.left()) % gridInterval);
		const qreal top = static_cast<int>(rect.top())
				- (static_cast<int>(rect.top()) % gridInterval);

		QVarLengthArray<QLineF, 100> linesX;
		for (qreal x = left; x < rect.right(); x += gridInterval)
			linesX.append(QLineF(x, rect.top(), x, rect.bottom()));

		QVarLengthArray<QLineF, 100> linesY;
		for (qreal y = top; y < rect.bottom(); y += gridInterval)
			linesY.append(QLineF(rect.left(), y, rect.right(), y));

		painter->drawLines(linesX.data(), linesX.size());
		painter->drawLines(linesY.data(), linesY.size());
	}
}

}
}
