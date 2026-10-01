// MIT License
//
// Copyright (c) 2024 Missing Deadlines (Benjamin Wrensch)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// All values used to derive this implementation are sourced from Troy's initial AgX implementation/OCIO config file available here:
//   https://github.com/sobotka/AgX

// https://iolite-engine.com/blog_posts/minimal_agx_implementation

// Mean error^2: 3.6705141e-06
vec3 agxDefaultContrastApprox(vec3 x) {
	vec3 x2 = x * x;
	vec3 x4 = x2 * x2;

	return + 15.5     * x4 * x2
			- 40.14    * x4 * x
			+ 31.96    * x4
			- 6.868    * x2 * x
			+ 0.4298   * x2
			+ 0.1191   * x
			- 0.00232;
}

vec3 agx(vec3 val) {
	const mat3 agx_mat = mat3(
		0.842479062253094, 0.0423282422610123, 0.0423756549057051,
		0.0784335999999992,  0.878468636469772,  0.0784336,
		0.0792237451477643, 0.0791661274605434, 0.879142973793104);

	const float min_ev = -12.47393f;
	const float max_ev = 4.026069f;

	// Input transform (inset)
	val = agx_mat * val;

	// Log2 space encoding
	val = clamp(log2(val), min_ev, max_ev);
	val = (val - min_ev) / (max_ev - min_ev);

	// Apply sigmoid function approximation
	val = agxDefaultContrastApprox(val);

	return val;
}

vec3 agxEotf(vec3 val) {
	const mat3 agx_mat_inv = mat3(
		1.19687900512017, -0.0528968517574562, -0.0529716355144438,
		-0.0980208811401368, 1.15190312990417, -0.0980434501171241,
		-0.0990297440797205, -0.0989611768448433, 1.15107367264116);

	// Inverse input transform (outset)
	val = agx_mat_inv * val;

	// sRGB IEC 61966-2-1 2.2 Exponent Reference EOTF Display
	// NOTE: We're linearizing the output here. Comment/adjust when
	// *not* using a sRGB render target
	val = pow(val, vec3(2.2));

	return val;
}

// look: 0 = Default, 1 = Golden, 2 = Punchy
vec3 agxLook(vec3 val, int look) {
	const vec3 lw = vec3(0.2126, 0.7152, 0.0722);
	float luma = dot(val, lw);

	// Default
	vec3 offset = vec3(0.0);
	vec3 slope = vec3(1.0);
	vec3 power = vec3(1.0);
	float sat = 1.0;

	if (look == 1) {
		// Golden
		slope = vec3(1.0, 0.9, 0.5);
		power = vec3(0.8);
		sat = 0.8;
	} else if (look == 2) {
		// Punchy
		slope = vec3(1.0);
		power = vec3(1.35, 1.35, 1.35);
		sat = 1.4;
	}

	// ASC CDL
	val = pow(val * slope + offset, power);
	return luma + sat * (val - luma);
}

// ACES (Academy Color Encoding System) filmic tone mapping curve.
// Fitted approximation by Krzysztof Narkowicz.
// Reference: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneAces(vec3 x) {
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Uncharted 2 / "Hable" filmic tone mapping operator.
// Reference: Hable, "Filmic Tonemapping Operators", GDC 2010 / Uncharted 2.
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 toneHable(vec3 x) {
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Reinhard tone mapping: maps [0, inf) to [0, 1).
// Reference: Reinhard et al., "Photographic Tone Reproduction for Digital Images", SIGGRAPH 2002.
vec3 toneReinhard(vec3 x) {
	return x / (1.0 + x);
}

// IEC 61966-2-1 sRGB transfer function (linear -> sRGB gamma encoding).
// Piecewise: linear segment below 0.0031308, gamma 2.4 segment above.
vec3 linearToSrgb(vec3 c) {
	vec3 lo = c * 12.92;
	vec3 hi = 1.055 * pow(max(c, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
	return mix(hi, lo, vec3(lessThanEqual(c, vec3(0.0031308))));
}

// mode: 0 = none, 1 = AgX, 2 = AgX golden, 3 = AgX punchy, 4 = ACES, 5 = Hable, 6 = Reinhard
vec3 tonemapping(vec3 value, int mode) {
	if (mode >= 1 && mode <= 3) {
		value = agx(value);
		value = agxLook(value, mode - 1);
		value = agxEotf(value);
		return value;
	}
	if (mode == 4) {
		return toneAces(value);
	}
	if (mode == 5) {
		return clamp(toneHable(value * 2.0) / toneHable(vec3(11.2)), 0.0, 1.0);
	}
	if (mode == 6) {
		return toneReinhard(value);
	}
	return clamp(value, 0.0, 1.0);
}
