/**
 * @file
 */

#include "tb_dimension.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"
#include "tb_types.h"
#include "tb_value.h"
#include <math.h>

namespace tb {

// == TBDimensionConverter ==================================================================================

void TBDimensionConverter::setDPI(int srcDpi, int dstDpi) {
	m_src_dpi = srcDpi;
	m_dst_dpi = dstDpi;
	m_dst_dpi_str.clear();
	if (needConversion()) {
		m_dst_dpi_str.setFormatted("@%d", m_dst_dpi);
	}
}

void TBDimensionConverter::getDstDPIFilename(const char *filename, TBTempBuffer *tempbuf) const {
	int dot_pos = 0;
	for (dot_pos = SDL_strlen(filename) - 1; dot_pos > 0; dot_pos--) {
		if (filename[dot_pos] == '.') {
			break;
		}
	}
	tempbuf->resetAppendPos();
	tempbuf->append(filename, dot_pos);
	tempbuf->appendString(getDstDPIStr());
	tempbuf->appendString(filename + dot_pos);
}

int TBDimensionConverter::dpToPx(int dp) const {
	return (int)roundf(dpToPxF((float)dp));
}

float TBDimensionConverter::dpToPxF(float dp) const {
	if (dp <= TB_INVALID_DIMENSION || dp == 0 || !needConversion()) {
		return dp;
	}
	return dp * m_dst_dpi / m_src_dpi;
}

int TBDimensionConverter::mmToPx(int mm) const {
	return (int)roundf(mmToPxF((float)mm));
}

float TBDimensionConverter::mmToPxF(float mm) const {
	if (mm <= TB_INVALID_DIMENSION || mm == 0) {
		return mm;
	}
	return mm * TBSystem::getDPI() / 25.4F;
}

int TBDimensionConverter::getPxFromString(const char *str, int defValue) const {
	if ((str == nullptr) || !is_start_of_number(str)) {
		return defValue;
	}
	const int len = SDL_strlen(str);
	const int val = SDL_atoi(str);
	if (len > 2 && SDL_strcmp(str + len - 2, "px") == 0) {
		return val;
	}
	if (len > 2 && SDL_strcmp(str + len - 2, "mm") == 0) {
		return mmToPx(val);
	}
	// "dp", unspecified or unknown unit is treated as dp.
	return dpToPx(val);
}

float TBDimensionConverter::getPxFromStringF(const char *str, float defValue) const {
	if ((str == nullptr) || !is_start_of_number(str)) {
		return defValue;
	}
	const int len = SDL_strlen(str);
	const float val = (float)SDL_atof(str);
	if (len > 2 && SDL_strcmp(str + len - 2, "px") == 0) {
		return val;
	}
	if (len > 2 && SDL_strcmp(str + len - 2, "mm") == 0) {
		return mmToPxF(val);
	}
	// "dp", unspecified or unknown unit is treated as dp.
	return dpToPxF(val);
}

int TBDimensionConverter::getPxFromValue(TBValue *value, int defValue) const {
	if (value == nullptr) {
		return defValue;
	}
	if (value->getType() == TBValue::TYPE_INT) {
		return dpToPx(value->getInt());
	}
	if (value->getType() == TBValue::TYPE_FLOAT) {
		return (int)roundf(dpToPxF(value->getFloat()));
	}
	return getPxFromString(value->getString(), defValue);
}

float TBDimensionConverter::getPxFromValueF(TBValue *value, float defValue) const {
	if (value == nullptr) {
		return defValue;
	}
	if (value->getType() == TBValue::TYPE_INT) {
		return dpToPxF((float)value->getInt());
	}
	if (value->getType() == TBValue::TYPE_FLOAT) {
		return dpToPxF(value->getFloat());
	}
	return getPxFromStringF(value->getString(), defValue);
}

} // namespace tb
