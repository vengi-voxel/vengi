/**
 * @file
 */

#include "Color.h"
#include "math/Vec4f.h"

namespace color {

const math::Vec4f &Clear() {
	static const math::Vec4f v = math::Vec4f(0.f, 0, 0, 0) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &White() {
	static const math::Vec4f v = math::Vec4f(255.f, 255, 255, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Black() {
	static const math::Vec4f v = math::Vec4f(0.f, 0, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Lime() {
	static const math::Vec4f v = math::Vec4f(109.f, 198, 2, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Pink() {
	static const math::Vec4f v = math::Vec4f(248.f, 4, 62, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &LightBlue() {
	static const math::Vec4f v = math::Vec4f(0.f, 153, 203, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &DarkBlue() {
	static const math::Vec4f v = math::Vec4f(55.f, 116, 145, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Orange() {
	static const math::Vec4f v = math::Vec4f(252.f, 167, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &LightYellow() {
	static const math::Vec4f v = math::Vec4f(127.f, 127, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Yellow() {
	static const math::Vec4f v = math::Vec4f(255.f, 255, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Sandy() {
	static const math::Vec4f v = math::Vec4f(237.f, 232, 160, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &LightGray() {
	static const math::Vec4f v = math::Vec4f(192.f, 192, 192, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Gray() {
	static const math::Vec4f v = math::Vec4f(128.f, 128, 128, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &DarkGray() {
	static const math::Vec4f v = math::Vec4f(84.f, 84, 84, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &LightRed() {
	static const math::Vec4f v = math::Vec4f(255.f, 96, 96, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Red() {
	static const math::Vec4f v = math::Vec4f(255.f, 0, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &DarkRed() {
	static const math::Vec4f v = math::Vec4f(128.f, 0, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &LightGreen() {
	static const math::Vec4f v = math::Vec4f(96.f, 255, 96, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Green() {
	static const math::Vec4f v = math::Vec4f(0.f, 255, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &DarkGreen() {
	static const math::Vec4f v = math::Vec4f(0.f, 128, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Blue() {
	static const math::Vec4f v = math::Vec4f(0.f, 0, 255, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &SteelBlue() {
	static const math::Vec4f v = math::Vec4f(35.f, 107, 142, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Olive() {
	static const math::Vec4f v = math::Vec4f(128.f, 128, 0, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Purple() {
	static const math::Vec4f v = math::Vec4f(128.f, 0, 128, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Cyan() {
	static const math::Vec4f v = math::Vec4f(0.f, 255, 255, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &Brown() {
	static const math::Vec4f v = math::Vec4f(107.f, 66, 38, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &LightBrown() {
	static const math::Vec4f v = math::Vec4f(150.f, 107, 72, 255) / math::Vec4f(magnitudef);
	return v;
}
const math::Vec4f &DarkBrown() {
	static const math::Vec4f v = math::Vec4f(82.f, 43, 26, 255) / math::Vec4f(magnitudef);
	return v;
}

} // namespace color
