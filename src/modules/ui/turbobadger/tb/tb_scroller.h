/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "tb_msg.h"

namespace tb {

class TBWidget;

/** TBScrollerFunction does the calculations of time, speed and distance
	that decides how the slow down of a scroll will happen.

	Note: Speed is in pixels per millisecond. Duration is in milliseconds
		  and distance is in pixels. Distance and speed may be negative! */
class TBScrollerFunction {
public:
	TBScrollerFunction(float decay) : m_decay(decay) {
	}

	/** Calculate the duration needed until the end distance is reached
		from the given start speed. */
	float getDurationFromSpeed(float start_speed);

	/** Calculate the start speed needed to reach the given distance. */
	float getSpeedFromDistance(float distance);

	/** Calculate the distance reached at the given elapsed_time_ms with the given start_speed. */
	float getDistanceAtTime(float start_speed, float elapsed_time_ms);

	/** Same as GetDistanceAtTime but rounded to integer. */
	int getDistanceAtTimeInt(float start_speed, float elapsed_time_ms);

private:
	float m_decay;
};

/** TBScrollerSnapListener may override the target scroll position of a TBScroller. */

class TBScrollerSnapListener {
public:
	virtual ~TBScrollerSnapListener(){};

	/** Called when the target scroll position is calculated.

		target_widget is the widget being scroller.
		target_x, target_y is the suggested target scroll position which may be changed
		to something else in this call.

		Note: The scroll positions are relative to the target widget (inner scrolled TBWidget).
			  If there's nested scrollable widgets, only the inner scrolled widget applies snapping. */
	virtual void onScrollSnap(TBWidget *target_widget, int &target_x, int &target_y) = 0;
};

/** TBScroller handles panning while the pointer is down and measure the pan
	speed over time. It also handles continued scrolling when the pointer has
	been released with a flick. */
class TBScroller : private TBMessageHandler {
public:
	TBScroller(TBWidget *target);
	~TBScroller();

	/** Set the listener that may override the target scroll position. */
	void setSnapListener(TBScrollerSnapListener *listener) {
		m_snap_listener = listener;
	}

	/** Start tracking pan movement from calls to OnPan. */
	void start();

	/** Stop tracking pan movement from calls to OnPan,
		or stop any ongoing scrolling. */
	void stop();

	/** Return true if the pan tracking is started or. */
	bool isStarted() const {
		return m_is_started;
	}

	/** Get the widget that will be panned/scrolled. Any parent of this
		widget may also be panned/scrolled. */
	TBWidget *getTarget() const {
		return m_target;
	}

	/** Pan the target widget (or any parent) with the given deltas.
		Should be called while the pointer is down.
		This will track the pan speed over time. */
	bool onPan(int dx, int dy);

	/** The panning ends and the scroller should start scrolling.
		Should be called when the pointer is released. */
	void onPanReleased();

	/** Start the scroller based on the given delta. Doesn't
		require previous calls to OnPan or OnPanReleased.

		If accumulative is true, the given delta will be
		added to any on going scroll. If it's false, any
		ongoing scroll will be canceled. */
	void onScrollBy(int dx, int dy, bool accumulative);

private:
	virtual void onMessageReceived(TBMessage *msg);
	bool isScrolling();
	bool stopIfAlmostStill();
	void stopOrSnapScroll();
	void reset();
	void adjustToSnappingAndScroll(float ppms_x, float ppms_y);
	void scroll(float start_speed_ppms_x, float start_speed_ppms_y);
	void getTargetChildTranslation(int &x, int &y) const;
	void getTargetScrollXY(int &x, int &y) const;
	TBWidget *m_target;
	TBScrollerSnapListener *m_snap_listener;
	TBScrollerFunction m_func;
	bool m_is_started;
	float m_pan_dx, m_pan_dy;
	float m_previous_pan_dx, m_previous_pan_dy;
	double m_pan_time_ms;
	double m_pan_delta_time_ms;
	float m_scroll_start_speed_ppms_x, m_scroll_start_speed_ppms_y;
	double m_scroll_start_ms;
	float m_scroll_duration_x_ms, m_scroll_duration_y_ms;
	int m_scroll_start_scroll_x, m_scroll_start_scroll_y;
	float m_pan_power_multiplier_x;
	float m_pan_power_multiplier_y;
	int m_expected_scroll_x;
	int m_expected_scroll_y;
};

} // namespace tb
