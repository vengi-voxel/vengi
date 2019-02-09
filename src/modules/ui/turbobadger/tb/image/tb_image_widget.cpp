/**
 * @file
 */

#include "image/tb_image_widget.h"
#include "tb_widgets_reader.h"
#include "tb_node_tree.h"

namespace tb {

PreferredSize TBImageWidget::OnCalculatePreferredContentSize(const SizeConstraints &constraints)
{
	return PreferredSize(m_image.Width(), m_image.Height());
}

void TBImageWidget::OnPaint(const PaintProps &paint_props)
{
	if (TBBitmapFragment *fragment = m_image.GetBitmap())
		g_renderer->DrawBitmap(GetPaddingRect(), TBRect(0, 0, m_image.Width(), m_image.Height()), fragment);
}

} // namespace tb
