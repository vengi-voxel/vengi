/**
 * @file
 */

#include "Color.h"

namespace color {

const glm::vec4 &Clear() {
	static const glm::vec4 v = glm::vec4(0.f, 0, 0, 0) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &White() {
	static const glm::vec4 v = glm::vec4(255.f, 255, 255, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Black() {
	static const glm::vec4 v = glm::vec4(0.f, 0, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Lime() {
	static const glm::vec4 v = glm::vec4(109.f, 198, 2, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Pink() {
	static const glm::vec4 v = glm::vec4(248.f, 4, 62, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &LightBlue() {
	static const glm::vec4 v = glm::vec4(0.f, 153, 203, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &DarkBlue() {
	static const glm::vec4 v = glm::vec4(55.f, 116, 145, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Orange() {
	static const glm::vec4 v = glm::vec4(252.f, 167, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &LightYellow() {
	static const glm::vec4 v = glm::vec4(127.f, 127, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Yellow() {
	static const glm::vec4 v = glm::vec4(255.f, 255, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Sandy() {
	static const glm::vec4 v = glm::vec4(237.f, 232, 160, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &LightGray() {
	static const glm::vec4 v = glm::vec4(192.f, 192, 192, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Gray() {
	static const glm::vec4 v = glm::vec4(128.f, 128, 128, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &DarkGray() {
	static const glm::vec4 v = glm::vec4(84.f, 84, 84, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &LightRed() {
	static const glm::vec4 v = glm::vec4(255.f, 96, 96, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Red() {
	static const glm::vec4 v = glm::vec4(255.f, 0, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &DarkRed() {
	static const glm::vec4 v = glm::vec4(128.f, 0, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &LightGreen() {
	static const glm::vec4 v = glm::vec4(96.f, 255, 96, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Green() {
	static const glm::vec4 v = glm::vec4(0.f, 255, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &DarkGreen() {
	static const glm::vec4 v = glm::vec4(0.f, 128, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Blue() {
	static const glm::vec4 v = glm::vec4(0.f, 0, 255, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &SteelBlue() {
	static const glm::vec4 v = glm::vec4(35.f, 107, 142, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Olive() {
	static const glm::vec4 v = glm::vec4(128.f, 128, 0, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Purple() {
	static const glm::vec4 v = glm::vec4(128.f, 0, 128, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Cyan() {
	static const glm::vec4 v = glm::vec4(0.f, 255, 255, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &Brown() {
	static const glm::vec4 v = glm::vec4(107.f, 66, 38, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &LightBrown() {
	static const glm::vec4 v = glm::vec4(150.f, 107, 72, 255) / glm::vec4(magnitudef);
	return v;
}
const glm::vec4 &DarkBrown() {
	static const glm::vec4 v = glm::vec4(82.f, 43, 26, 255) / glm::vec4(magnitudef);
	return v;
}

} // namespace color
