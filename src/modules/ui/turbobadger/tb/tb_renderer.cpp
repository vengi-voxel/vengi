/**
 * @file
 */

#include "tb_renderer.h"

namespace tb {

void TBRenderer::invokeContextLost()
{
	TBLinkListOf<TBRendererListener>::Iterator iter = m_listeners.iterateForward();
	while (TBRendererListener *listener = iter.getAndStep())
		listener->onContextLost();
}

void TBRenderer::invokeContextRestored()
{
	TBLinkListOf<TBRendererListener>::Iterator iter = m_listeners.iterateForward();
	while (TBRendererListener *listener = iter.getAndStep())
		listener->onContextRestored();
}

} // namespace tb
