/**
 * @file
 */

#include "tb_renderer.h"

namespace tb {

void TBRenderer::InvokeContextLost()
{
	TBLinkListOf<TBRendererListener>::Iterator iter = m_listeners.IterateForward();
	while (TBRendererListener *listener = iter.GetAndStep())
		listener->OnContextLost();
}

void TBRenderer::InvokeContextRestored()
{
	TBLinkListOf<TBRendererListener>::Iterator iter = m_listeners.IterateForward();
	while (TBRendererListener *listener = iter.GetAndStep())
		listener->OnContextRestored();
}

} // namespace tb
