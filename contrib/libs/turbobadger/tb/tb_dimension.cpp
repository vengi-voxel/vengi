// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_dimension.h"
#include "tb_types.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"
#include "tb_value.h"
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

namespace tb {

// == TBDimensionConverter ==================================================================================

void TBDimensionConverter::SetDPI(int src_dpi, int dst_dpi)
{
	m_src_dpi = src_dpi;
	m_dst_dpi = dst_dpi;
	m_dst_dpi_str.Clear();
	if (NeedConversion())
		m_dst_dpi_str.SetFormatted("@%d", m_dst_dpi);
}

void TBDimensionConverter::GetDstDPIFilename(const char *filename, TBTempBuffer *tempbuf) const
{
	int dot_pos = 0;
	for (dot_pos = strlen(filename) - 1; dot_pos > 0; dot_pos--)
		if (filename[dot_pos] == '.')
			break;
	tempbuf->ResetAppendPos();
	tempbuf->Append(filename, dot_pos);
	tempbuf->AppendString(GetDstDPIStr());
	tempbuf->AppendString(filename + dot_pos);
}

int TBDimensionConverter::DpToPx(int dp) const
{
	return (int) roundf(DpToPxF((float) dp));
}

float TBDimensionConverter::DpToPxF(float dp) const
{
	if (dp <= TB_INVALID_DIMENSION || dp == 0 || !NeedConversion())
		return dp;
	return dp * m_dst_dpi / m_src_dpi;
}

int TBDimensionConverter::MmToPx(int mm) const
{
	return (int) roundf(MmToPxF((float) mm));
}

float TBDimensionConverter::MmToPxF(float mm) const
{
	if (mm <= TB_INVALID_DIMENSION || mm == 0)
		return mm;
	return mm * TBSystem::GetDPI() / 25.4f;
}

int TBDimensionConverter::GetPxFromString(const char *str, int def_value) const
{
	if (!str || !is_start_of_number(str))
		return def_value;
	const int len = strlen(str);
	const int val = atoi(str);
	if (len > 2 && strcmp(str + len - 2, "px") == 0)
		return val;
	if (len > 2 && strcmp(str + len - 2, "mm") == 0)
		return MmToPx(val);
	// "dp", unspecified or unknown unit is treated as dp.
	return DpToPx(val);
}

float TBDimensionConverter::GetPxFromStringF(const char *str, float def_value) const
{
	if (!str || !is_start_of_number(str))
		return def_value;
	const int len = strlen(str);
	const float val = (float) atof(str);
	if (len > 2 && strcmp(str + len - 2, "px") == 0)
		return val;
	if (len > 2 && strcmp(str + len - 2, "mm") == 0)
		return MmToPxF(val);
	// "dp", unspecified or unknown unit is treated as dp.
	return DpToPxF(val);
}

int TBDimensionConverter::GetPxFromValue(TBValue *value, int def_value) const
{
	if (!value)
		return def_value;
	if (value->GetType() == TBValue::TYPE_INT)
		return DpToPx(value->GetInt());
	else if (value->GetType() == TBValue::TYPE_FLOAT)
		return (int) roundf(DpToPxF(value->GetFloat()));
	return GetPxFromString(value->GetString(), def_value);
}

float TBDimensionConverter::GetPxFromValueF(TBValue *value, float def_value) const
{
	if (!value)
		return def_value;
	if (value->GetType() == TBValue::TYPE_INT)
		return DpToPxF((float) value->GetInt());
	else if (value->GetType() == TBValue::TYPE_FLOAT)
		return DpToPxF(value->GetFloat());
	return GetPxFromStringF(value->GetString(), def_value);
}

} // namespace tb
