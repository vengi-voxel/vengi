/**
 * @file
 */

#include "tb_scroller.h"
#include "tb_system.h"
#include "tb_widgets.h"
#include <math.h>

namespace tb {

// == Misc constants ====================================================================

#define PAN_TARGET_FPS 60
#define PAN_MSG_DELAY_MS ((double)(1000.0 / PAN_TARGET_FPS))

#define PAN_START_THRESHOLD_MS 50
#define PAN_POWER_ACC_THRESHOLD_MS 600

#define PAN_POWER_MULTIPLIER 1.3f

#define SCROLL_DECAY 200.0f

#define SF_GATE_THRESHOLD 0.01f

// Lab:
// http://www.madtealab.com/?V=1&C=6&F=5&G=1&O=1&W=774&GW=720&GH=252&GX=13.389616776278201&GY=4.790704772336853&GS=0.13102127484993598&EH=189&a=3.6666666666666665&aMa=20&aN=OrgSpeed&bMa=3&bN=CurPos&c=8&cMa=60&cI=1&cN=FrameRate&d=16&dMa=16&dI=1&dN=numSimulatedSeconds&l=2.388888888888889&lMa=5&lN=Decay&m=0.1&mMa=0.1&mN=GateThreshold&f1=OrgSpeed+%2A+exp%28-x+%2F+Decay%29&f1N=Speed&f2=CurPos+%2B+OrgSpeed+%2A+%281-exp%28-x+%2F+Decay%29%29%2A+Decay&f2N=Pos&f3=marker%28x%2C+predictGatedPoint%29&f3N=GatePoint&f4=aToF%28simulatedPoints%2Cnearest%2C0%2CnumSimulatedSeconds%29%28x%29&f4N=Iterated&f5=OrgSpeed+%2A+x&f5N=Linear1&Expr=%0ApredictGatedPoint+%3D+-log%28GateThreshold+%2F+%28OrgSpeed%29%29+%2A+Decay%0A%0Avar+cur+%3D+OrgSpeed%0AsimulatedPoints+%3D+sample%28function%28%29+%7B%0A+++cur+%3D+cur+%2A+%281+-+0.05%29%3B%0A+++return+cur%0A+%7D%2C+%5BnumSimulatedSeconds+%2A+FrameRate%5D%29%3B%0A%0ApredictGatedPoint

float TBScrollerFunction::getDurationFromSpeed(float startSpeed) {
	float abs_start_speed = Abs(startSpeed);
	if (abs_start_speed <= SF_GATE_THRESHOLD) {
		return 0;
	}
	return -logf(SF_GATE_THRESHOLD / abs_start_speed) * m_decay;
}

float TBScrollerFunction::getSpeedFromDistance(float distance) {
	float speed = distance / m_decay;
	if (distance > SF_GATE_THRESHOLD) {
		return speed + SF_GATE_THRESHOLD;
	}
	if (distance < -SF_GATE_THRESHOLD) {
		return speed - SF_GATE_THRESHOLD;
	}
	return speed;
}

float TBScrollerFunction::getDistanceAtTime(float startSpeed, float elapsedTimeMs) {
	core_assert(elapsedTimeMs >= 0);
	return startSpeed * (1 - expf(-elapsedTimeMs / m_decay)) * m_decay;
}

int TBScrollerFunction::getDistanceAtTimeInt(float startSpeed, float elapsedTimeMs) {
	float distance = getDistanceAtTime(startSpeed, elapsedTimeMs);
	return (int)(distance < 0 ? distance - 0.5F : distance + 0.5F);
}

// == TBScroller ========================================================================

TBScroller::TBScroller(TBWidget *target)
	: m_target(target), m_snap_listener(nullptr), m_func(SCROLL_DECAY), m_previous_pan_dx(0), m_previous_pan_dy(0),
	  m_scroll_start_ms(0), m_scroll_duration_x_ms(0), m_scroll_duration_y_ms(0), m_pan_power_multiplier_x(1),
	  m_pan_power_multiplier_y(1) {
	reset();
}

TBScroller::~TBScroller() {
}

void TBScroller::reset() {
	m_is_started = false;
	m_pan_dx = m_pan_dy = 0;
	m_pan_time_ms = 0;
	m_pan_delta_time_ms = 0;
	m_scroll_start_speed_ppms_x = m_scroll_start_speed_ppms_y = 0;
	m_scroll_start_scroll_x = m_scroll_start_scroll_y = 0;
	// don't reset m_previous_pan_dx and m_previous_pan_dy here.
	// don't reset m_pan_power here. It's done on start since it's needed for next pan!
	m_expected_scroll_x = m_expected_scroll_y = 0;
}

void TBScroller::onScrollBy(int dx, int dy, bool accumulative) {
	if (!isStarted()) {
		start();
	}

	float ppms_x = m_func.getSpeedFromDistance((float)dx);
	float ppms_y = m_func.getSpeedFromDistance((float)dy);

	if (accumulative && isScrolling()) {
		TBWidget::ScrollInfo info = m_target->getScrollInfo();
		// If new direction is the same as the current direction,
		// calculate the speed needed for the remaining part and
		// add that to the new scroll speed.
		if ((ppms_x < 0) == (m_scroll_start_speed_ppms_x < 0)) {
			int distance_x = m_func.getDistanceAtTimeInt(m_scroll_start_speed_ppms_x,
														 m_func.getDurationFromSpeed(m_scroll_start_speed_ppms_x));
			int distance_remaining_x = m_scroll_start_scroll_x + distance_x - info.x;
			distance_remaining_x += m_func.getDistanceAtTimeInt(ppms_x, m_func.getDurationFromSpeed(ppms_x));
			ppms_x = m_func.getSpeedFromDistance((float)distance_remaining_x);
		}
		if ((ppms_y < 0) == (m_scroll_start_speed_ppms_y < 0)) {
			int distance_y = m_func.getDistanceAtTimeInt(m_scroll_start_speed_ppms_y,
														 m_func.getDurationFromSpeed(m_scroll_start_speed_ppms_y));
			int distance_remaining_y = m_scroll_start_scroll_y + distance_y - info.y;
			distance_remaining_y += m_func.getDistanceAtTimeInt(ppms_y, m_func.getDurationFromSpeed(ppms_y));
			ppms_y = m_func.getSpeedFromDistance((float)distance_remaining_y);
		}
	}

	adjustToSnappingAndScroll(ppms_x, ppms_y);
}

bool TBScroller::onPan(int dx, int dy) {
	if (!isStarted()) {
		start();
	}

	// Pan the target
	const int in_dx = dx;
	const int in_dy = dy;
	m_target->scrollByRecursive(dx, dy);

	// Calculate the pan speed. Smooth it out with the
	// previous pan speed to reduce fluctuation a little.
	double now_ms = TBSystem::getTimeMS();
	if (m_pan_time_ms != 0.0) {
		if (m_pan_delta_time_ms != 0.0) {
			m_pan_delta_time_ms = (now_ms - m_pan_time_ms + m_pan_delta_time_ms) / 2.0F;
		} else {
			m_pan_delta_time_ms = now_ms - m_pan_time_ms;
		}
	}

	m_pan_time_ms = now_ms;
	m_pan_dx = (m_pan_dx + in_dx) / 2.0F;
	m_pan_dy = (m_pan_dy + in_dy) / 2.0F;

	// If we change direction, reset the pan power multiplier in that axis.
	if (m_pan_dx != 0 && (m_previous_pan_dx < 0) != (m_pan_dx < 0)) {
		m_pan_power_multiplier_x = 1;
	}
	if (m_pan_dy != 0 && (m_previous_pan_dy < 0) != (m_pan_dy < 0)) {
		m_pan_power_multiplier_y = 1;
	}
	m_previous_pan_dx = m_pan_dx;
	m_previous_pan_dy = m_pan_dy;

	return in_dx != dx || in_dy != dy;
}

void TBScroller::onPanReleased() {
	if (TBSystem::getTimeMS() < m_pan_time_ms + PAN_START_THRESHOLD_MS) {
		// Don't start scroll if we have too little speed.
		// This will prevent us from scrolling accidently.
		float pan_start_distance_threshold_px = 2 * TBSystem::getDPI() / 100.0F;
		if (Abs(m_pan_dx) < pan_start_distance_threshold_px && Abs(m_pan_dy) < pan_start_distance_threshold_px) {
			stopOrSnapScroll();
			return;
		}

		if (m_pan_delta_time_ms == 0) {
			stopOrSnapScroll();
			return;
		}

		float ppms_x = (float)m_pan_dx / (float)m_pan_delta_time_ms;
		float ppms_y = (float)m_pan_dy / (float)m_pan_delta_time_ms;
		ppms_x *= m_pan_power_multiplier_x;
		ppms_y *= m_pan_power_multiplier_y;

		adjustToSnappingAndScroll(ppms_x, ppms_y);
	} else {
		stopOrSnapScroll();
	}
}

void TBScroller::start() {
	if (isStarted()) {
		return;
	}
	m_is_started = true;
	double now_ms = TBSystem::getTimeMS();
	if (now_ms < m_scroll_start_ms + PAN_POWER_ACC_THRESHOLD_MS) {
		m_pan_power_multiplier_x *= PAN_POWER_MULTIPLIER;
		m_pan_power_multiplier_y *= PAN_POWER_MULTIPLIER;
	} else {
		m_pan_power_multiplier_x = m_pan_power_multiplier_y = 1;
	}
}

void TBScroller::stop() {
	deleteAllMessages();
	reset();
}

bool TBScroller::stopIfAlmostStill() {
	double now_ms = TBSystem::getTimeMS();
	if (now_ms > m_scroll_start_ms + (double)m_scroll_duration_x_ms &&
		now_ms > m_scroll_start_ms + (double)m_scroll_duration_y_ms) {
		stop();
		return true;
	}
	return false;
}

void TBScroller::stopOrSnapScroll() {
	adjustToSnappingAndScroll(0, 0);
	if (!isScrolling()) {
		stop();
	}
}

void TBScroller::adjustToSnappingAndScroll(float ppmsX, float ppmsY) {
	if (m_snap_listener != nullptr) {
		// Calculate the distance
		int distance_x = m_func.getDistanceAtTimeInt(ppmsX, m_func.getDurationFromSpeed(ppmsX));
		int distance_y = m_func.getDistanceAtTimeInt(ppmsY, m_func.getDurationFromSpeed(ppmsY));

		// Let the snap listener modify the distance
		TBWidget::ScrollInfo info = m_target->getScrollInfo();
		int target_x = distance_x + info.x;
		int target_y = distance_y + info.y;
		m_snap_listener->onScrollSnap(m_target, target_x, target_y);
		distance_x = target_x - info.x;
		distance_y = target_y - info.y;

		// Get the start speed from the new distance
		ppmsX = m_func.getSpeedFromDistance((float)distance_x);
		ppmsY = m_func.getSpeedFromDistance((float)distance_y);
	}

	scroll(ppmsX, ppmsY);
}

void TBScroller::scroll(float startSpeedPpmsX, float startSpeedPpmsY) {
	// Set start values
	m_scroll_start_ms = TBSystem::getTimeMS();
	getTargetScrollXY(m_scroll_start_scroll_x, m_scroll_start_scroll_y);
	m_scroll_start_speed_ppms_x = startSpeedPpmsX;
	m_scroll_start_speed_ppms_y = startSpeedPpmsY;

	// Calculate duration for the scroll (each axis independently)
	m_scroll_duration_x_ms = m_func.getDurationFromSpeed(m_scroll_start_speed_ppms_x);
	m_scroll_duration_y_ms = m_func.getDurationFromSpeed(m_scroll_start_speed_ppms_y);

	if (stopIfAlmostStill()) {
		return;
	}

	// Post the pan message if we don't already have one
	if (getMessageByID(TBIDC("scroll")) == nullptr) {
		// Update expected translation
		getTargetChildTranslation(m_expected_scroll_x, m_expected_scroll_y);

		postMessageDelayed(TBIDC("scroll"), nullptr, (uint32_t)PAN_MSG_DELAY_MS);
	}
}

bool TBScroller::isScrolling() {
	return getMessageByID(TBIDC("scroll")) != nullptr;
}

void TBScroller::getTargetChildTranslation(int &x, int &y) const {
	int root_x = 0;
	int root_y = 0;
	int child_translation_x = 0;
	int child_translation_y = 0;
	TBWidget *scroll_root = m_target->getScrollRoot();
	scroll_root->convertToRoot(root_x, root_y);
	scroll_root->getChildTranslation(child_translation_x, child_translation_y);
	x = root_x + child_translation_x;
	y = root_y + child_translation_y;
}

void TBScroller::getTargetScrollXY(int &x, int &y) const {
	x = 0;
	y = 0;
	TBWidget *tmp = m_target->getScrollRoot();
	while (tmp != nullptr) {
		TBWidget::ScrollInfo info = tmp->getScrollInfo();
		x += info.x;
		y += info.y;
		tmp = tmp->getParent();
	}
}

void TBScroller::onMessageReceived(TBMessage *msg) {
	if (msg->message == TBIDC("scroll")) {
		int actual_scroll_x = 0;
		int actual_scroll_y = 0;
		getTargetChildTranslation(actual_scroll_x, actual_scroll_y);
		if (actual_scroll_x != m_expected_scroll_x || actual_scroll_y != m_expected_scroll_y) {
			// Something else has affected the target child translation.
			// This should abort the scroll.
			// This could happen f.ex if something shrunk the scroll limits,
			// some other action changed scroll position, or if another
			// scroller started operating on a sub child that when reacing
			// its scroll limit, started scrolling its chain of parents.
			stop();
			return;
		}

		// Calculate the time elapsed from scroll start. Clip within the
		// duration for each axis.
		double now_ms = TBSystem::getTimeMS();
		float elapsed_time_x = (float)(now_ms - m_scroll_start_ms);
		float elapsed_time_y = elapsed_time_x;
		elapsed_time_x = Min(elapsed_time_x, m_scroll_duration_x_ms);
		elapsed_time_y = Min(elapsed_time_y, m_scroll_duration_y_ms);

		// Get the new scroll position from the current distance in each axis.
		int scroll_x = m_func.getDistanceAtTimeInt(m_scroll_start_speed_ppms_x, elapsed_time_x);
		int scroll_y = m_func.getDistanceAtTimeInt(m_scroll_start_speed_ppms_y, elapsed_time_y);
		scroll_x += m_scroll_start_scroll_x;
		scroll_y += m_scroll_start_scroll_y;

		// Get the scroll delta and invoke ScrollByRecursive.
		int curr_scroll_x;
		int curr_scroll_y;
		getTargetScrollXY(curr_scroll_x, curr_scroll_y);
		const int dx = scroll_x - curr_scroll_x;
		const int dy = scroll_y - curr_scroll_y;

		int idx = dx;
		int idy = dy;
		m_target->scrollByRecursive(idx, idy);

		// Update expected translation
		getTargetChildTranslation(m_expected_scroll_x, m_expected_scroll_y);

		if (((dx != 0) && actual_scroll_x == m_expected_scroll_x) &&
			((dy != 0) && actual_scroll_y == m_expected_scroll_y)) {
			// We didn't get anywhere despite we tried,
			// so we're done (reached the end).
			stop();
			return;
		}

		if (!stopIfAlmostStill()) {
			double next_fire_time = msg->getFireTime() + PAN_MSG_DELAY_MS;
			// avoid timer catch-up if program went sleeping for a while.
			next_fire_time = Max(next_fire_time, now_ms);
			postMessageOnTime(TBIDC("scroll"), nullptr, next_fire_time);
		}
	}
}

} // namespace tb
