/**
 * @file
 */

#include "animation/tb_widget_animation.h"
#include "tb_window.h"
#include "tb_widgets.h"
#include "tb_widgets_common.h"
#include "tb_message_window.h"
#include "tb_list.h"

namespace tb {

TBLinkListOf<TBWidgetAnimationObject> widget_animations;

#define LERP(src, dst, progress) (src + (dst - src) * progress)

TBWidgetAnimationObject::TBWidgetAnimationObject(TBWidget *widget)
	: m_widget(widget)
{
	widget_animations.addLast(this);
}

TBWidgetAnimationObject::~TBWidgetAnimationObject()
{
	widget_animations.remove(this);
}

// == TBWidgetAnimationOpacity ============================================================

TBWidgetAnimationOpacity::TBWidgetAnimationOpacity(TBWidget *widget, float srcOpacity, float dstOpacity, bool die)
	: TBWidgetAnimationObject(widget)
	, m_src_opacity(srcOpacity)
	, m_dst_opacity(dstOpacity)
	, m_die(die)
{
}

void TBWidgetAnimationOpacity::onAnimationStart()
{
	// Make sure we don't stay idle if nothing is scheduled (hack).
	// FIX: fix this properly
	m_widget->invalidate();

	m_widget->setOpacity(m_src_opacity);
}

void TBWidgetAnimationOpacity::onAnimationUpdate(float progress)
{
	m_widget->setOpacity(LERP(m_src_opacity, m_dst_opacity, progress));
}

void TBWidgetAnimationOpacity::onAnimationStop(bool aborted)
{
	// If we're aborted, it may be because the widget is being deleted
	if (m_die && !aborted)
	{
		TBWidgetSafePointer the_widget(m_widget);
		m_widget->removeFromParent();
		if (the_widget.get())
			delete the_widget.get();
	}
	else
		m_widget->setOpacity(m_dst_opacity);
}

TBWidgetAnimationRect::TBWidgetAnimationRect(TBWidget *widget, const TBRect &srcRect, const TBRect &dstRect)
	: TBWidgetAnimationObject(widget)
	, m_src_rect(srcRect)
	, m_dst_rect(dstRect)
	, m_mode(MODE_SRC_TO_DST)
{
}

TBWidgetAnimationRect::TBWidgetAnimationRect(TBWidget *widget, const TBRect &deltaRect, MODE mode)
	: TBWidgetAnimationObject(widget)
	, m_delta_rect(deltaRect)
	, m_mode(mode)
{
	core_assert(mode == MODE_DELTA_IN || mode == MODE_DELTA_OUT);
}

void TBWidgetAnimationRect::onAnimationStart()
{
	// Make sure we don't stay idle if nothing is scheduled (hack).
	// FIX: fix this properly
	m_widget->invalidate();

	if (m_mode == MODE_SRC_TO_DST)
		m_widget->setRect(m_src_rect);
}

void TBWidgetAnimationRect::onAnimationUpdate(float progress)
{
	if (m_mode == MODE_DELTA_IN || m_mode == MODE_DELTA_OUT)
	{
		m_dst_rect = m_src_rect = m_widget->getRect();
		if (m_dst_rect.equals(TBRect()))
		{
			// Widget hasn't been laid out yet,
			// the animation was started too soon.
			//! \TODO this is certainly a BUG because it can be called from within the
			// 		TBAnimationManager::update() loop which ALSO deletes the animation objevt.
			TBAnimationManager::abortAnimation(this, true);
			return;
		}
		if (m_mode == MODE_DELTA_IN)
		{
			m_dst_rect.x += m_delta_rect.x;
			m_dst_rect.y += m_delta_rect.y;
			m_dst_rect.w += m_delta_rect.w;
			m_dst_rect.h += m_delta_rect.h;
		}
		else
		{
			m_src_rect.x += m_delta_rect.x;
			m_src_rect.y += m_delta_rect.y;
			m_src_rect.w += m_delta_rect.w;
			m_src_rect.h += m_delta_rect.h;
		}
		m_mode = MODE_SRC_TO_DST;
	}
	TBRect rect;
	rect.x = (int) LERP(m_src_rect.x, m_dst_rect.x, progress);
	rect.y = (int) LERP(m_src_rect.y, m_dst_rect.y, progress);
	rect.w = (int) LERP(m_src_rect.w, m_dst_rect.w, progress);
	rect.h = (int) LERP(m_src_rect.h, m_dst_rect.h, progress);
	m_widget->setRect(rect);
}

void TBWidgetAnimationRect::onAnimationStop(bool aborted)
{
	if (m_mode == MODE_SRC_TO_DST) // m_dst_rect may still be unset if aborted.
		m_widget->setRect(m_dst_rect);
}

// == TBWidgetsAnimationManager =====================================================

TBWidgetsAnimationManager widgets_animation_manager;

void TBWidgetsAnimationManager::init()
{
	TBWidgetListener::addGlobalListener(&widgets_animation_manager);
}

void TBWidgetsAnimationManager::shutdown()
{
	TBWidgetListener::removeGlobalListener(&widgets_animation_manager);
}

void TBWidgetsAnimationManager::abortAnimations(TBWidget *widget)
{
	abortAnimations(widget, nullptr);
}

void TBWidgetsAnimationManager::abortAnimations(TBWidget *widget, TB_TYPE_ID typeId)
{
	TBLinkListOf<TBWidgetAnimationObject>::Iterator iter = widget_animations.iterateForward();
	while (TBWidgetAnimationObject *wao = iter.getAndStep())
	{
		if (wao->m_widget == widget)
		{
			// Skip this animation if we asked for a specific (and
			// different) animation type.
			if (typeId != nullptr && !wao->isOfTypeId(typeId))
				continue;

			// Abort the animation. This will both autoremove itself
			// and delete it, so no need to do it here.
			TBAnimationManager::abortAnimation(wao, true);
		}
	}
}

void TBWidgetsAnimationManager::onWidgetDelete(TBWidget *widget)
{
	// Kill and delete all animations running for the widget being deleted.
	abortAnimations(widget);
}

bool TBWidgetsAnimationManager::onWidgetDying(TBWidget *widget)
{
	bool handled = false;
	if (TBWindow *window = TBSafeCast<TBWindow>(widget))
	{
		// Fade out dying windows
		if (TBAnimationObject *anim = new TBWidgetAnimationOpacity(window, 1.f, TB_ALMOST_ZERO_OPACITY, true))
			TBAnimationManager::startAnimation(anim, ANIMATION_CURVE_BEZIER);
		handled = true;
	}
	if (TBMessageWindow *window = TBSafeCast<TBMessageWindow>(widget))
	{
		// Move out dying message windows
		if (TBAnimationObject *anim = new TBWidgetAnimationRect(window, TBRect(0, 50, 0, 0), TBWidgetAnimationRect::MODE_DELTA_IN))
			TBAnimationManager::startAnimation(anim, ANIMATION_CURVE_SPEED_UP);
		handled = true;
	}
	if (TBDimmer *dimmer = TBSafeCast<TBDimmer>(widget))
	{
		// Fade out dying dim layers
		if (TBAnimationObject *anim = new TBWidgetAnimationOpacity(dimmer, 1.f, TB_ALMOST_ZERO_OPACITY, true))
			TBAnimationManager::startAnimation(anim, ANIMATION_CURVE_BEZIER);
		handled = true;
	}
	return handled;
}

void TBWidgetsAnimationManager::onWidgetAdded(TBWidget *parent, TBWidget *widget)
{
	if (TBWindow *window = TBSafeCast<TBWindow>(widget))
	{
		// Fade in new windows
		if (TBAnimationObject *anim = new TBWidgetAnimationOpacity(window, TB_ALMOST_ZERO_OPACITY, 1.f, false))
			TBAnimationManager::startAnimation(anim, ANIMATION_CURVE_BEZIER);
	}
	if (TBMessageWindow *window = TBSafeCast<TBMessageWindow>(widget))
	{
		// Move in new message windows
		if (TBAnimationObject *anim = new TBWidgetAnimationRect(window, TBRect(0, -50, 0, 0), TBWidgetAnimationRect::MODE_DELTA_OUT))
			TBAnimationManager::startAnimation(anim);
	}
	if (TBDimmer *dimmer = TBSafeCast<TBDimmer>(widget))
	{
		// Fade in dim layer
		if (TBAnimationObject *anim = new TBWidgetAnimationOpacity(dimmer, TB_ALMOST_ZERO_OPACITY, 1.f, false))
			TBAnimationManager::startAnimation(anim, ANIMATION_CURVE_BEZIER);
	}
}

void TBWidgetsAnimationManager::onWidgetRemove(TBWidget *parent, TBWidget *widget)
{
}

} // namespace tb
