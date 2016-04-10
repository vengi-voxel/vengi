#pragma once

namespace Cubiquity {

class Brush {
public:
	Brush(float innerRadius, float outerRadius, float opacity);

	float innerRadius(void) const;
	float outerRadius(void) const;
	float opacity(void) const;

	void setInnerRadius(float value);
	void setOuterRadius(float value);
	void setOpacity(float value);

private:
	float _innerRadius;
	float _outerRadius;
	float _opacity;
};

}
