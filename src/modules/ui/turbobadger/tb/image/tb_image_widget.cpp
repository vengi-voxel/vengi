/**
 * @file
 */

#include "image/tb_image_widget.h"
#include "tb_node_tree.h"
#include "tb_widgets_reader.h"

namespace tb {

PreferredSize TBImageWidget::onCalculatePreferredContentSize(const SizeConstraints &constraints) {
	return PreferredSize(m_image.width(), m_image.height());
}

void TBImageWidget::onPaint(const PaintProps &paintProps) {
	if (TBBitmapFragment *fragment = m_image.getBitmap())
		g_renderer->drawBitmap(getPaddingRect(), TBRect(0, 0, m_image.width(), m_image.height()), fragment);
}

} // namespace tb
