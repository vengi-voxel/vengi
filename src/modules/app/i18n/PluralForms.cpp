/**
 * @file
 */

// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2006 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "PluralForms.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicStringMap.h"

namespace app {

namespace {

/**
 *  Plural functions are used to select a string that matches a given
 *  count. \a n is the count and the return value is the string index
 *  used in the .po file, for example:
 *
 *   msgstr[0] = "You got %d error";
 *   msgstr[1] = "You got %d errors";
 *          ^-- return value of plural function
 */
unsigned int plural1(int) {
	return 0;
}
unsigned int plural2_1(int n) {
	return (n != 1);
}
unsigned int plural2_2(int n) {
	return (n > 1);
}
unsigned int plural2_mk(int n) {
	return n == 1 || n % 10 == 1 ? 0 : 1;
}
unsigned int plural2_mk_2(int n) {
	return static_cast<unsigned int>((n % 10 == 1 && n % 100 != 11) ? 0 : 1);
}
unsigned int plural3_lv(int n) {
	return static_cast<unsigned int>(n % 10 == 1 && n % 100 != 11 ? 0 : n != 0 ? 1 : 2);
}
unsigned int plural3_ga(int n) {
	return static_cast<unsigned int>(n == 1 ? 0 : n == 2 ? 1 : 2);
}
unsigned int plural3_lt(int n) {
	return static_cast<unsigned int>(n % 10 == 1 && n % 100 != 11					  ? 0
									 : n % 10 >= 2 && (n % 100 < 10 || n % 100 >= 20) ? 1
																					  : 2);
}
unsigned int plural3_1(int n) {
	return static_cast<unsigned int>(n % 10 == 1 && n % 100 != 11									 ? 0
									 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20) ? 1
																									 : 2);
}
unsigned int plural3_sk(int n) {
	return static_cast<unsigned int>((n == 1) ? 0 : (n >= 2 && n <= 4) ? 1 : 2);
}
unsigned int plural3_pl(int n) {
	return static_cast<unsigned int>(n == 1															 ? 0
									 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20) ? 1
																									 : 2);
}
unsigned int plural3_ro(int n) {
	return static_cast<unsigned int>(n == 1 ? 0 : (((n % 100 > 19) || ((n % 100 == 0) && (n != 0))) ? 2 : 1));
}
unsigned int plural3_sl(int n) {
	return static_cast<unsigned int>(n % 100 == 1 ? 0 : n % 100 == 2 ? 1 : n % 100 == 3 || n % 100 == 4 ? 2 : 3);
}
unsigned int plural4_be(int n) {
	return static_cast<unsigned int>(n % 10 == 1 && n % 100 != 11									? 0
									 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14) ? 1
									 : n % 10 == 0 || (n % 10 >= 5 && n % 10 <= 9) || (n % 100 >= 11 && n % 100 <= 14)
										 ? 2
										 : 3);
}
unsigned int plural4_cs(int n) {
	return static_cast<unsigned int>((n == 1 && n % 1 == 0)				? 0
									 : (n >= 2 && n <= 4 && n % 1 == 0) ? 1
									 : (n % 1 != 0)						? 2
																		: 3);
}
unsigned int plural4_cy(int n) {
	return static_cast<unsigned int>((n == 1) ? 0 : (n == 2) ? 1 : (n != 8 && n != 11) ? 2 : 3);
}
unsigned int plural4_gd(int n) {
	return static_cast<unsigned int>((n == 1 || n == 11) ? 0 : (n == 2 || n == 12) ? 1 : (n > 2 && n < 20) ? 2 : 3);
}
unsigned int plural4_he(int n) {
	return static_cast<unsigned int>((n == 1 && n % 1 == 0)					 ? 0
									 : (n == 2 && n % 1 == 0)				 ? 1
									 : (n % 10 == 0 && n % 1 == 0 && n > 10) ? 2
																			 : 3);
}
unsigned int plural4_lt(int n) {
	return static_cast<unsigned int>(n % 10 == 1 && (n % 100 > 19 || n % 100 < 11)					  ? 0
									 : (n % 10 >= 2 && n % 10 <= 9) && (n % 100 > 19 || n % 100 < 11) ? 1
									 : n % 1 != 0													  ? 2
																									  : 3);
}
unsigned int plural4_pl(int n) {
	return static_cast<unsigned int>(n == 1															  ? 0
									 : (n % 10 >= 2 && n % 10 <= 4) && (n % 100 < 12 || n % 100 > 14) ? 1
									 : (n != 1 && (n % 10 >= 0 && n % 10 <= 1)) || (n % 10 >= 5 && n % 10 <= 9) ||
											 (n % 100 >= 12 && n % 100 <= 14)
										 ? 2
										 : 3);
}
unsigned int plural4_sk(int n) {
	return static_cast<unsigned int>(n % 1 == 0 && n == 1			  ? 0
									 : n % 1 == 0 && n >= 2 && n <= 4 ? 1
									 : n % 1 != 0					  ? 2
																	  : 3);
}
unsigned int plural4_uk(int n) {
	return static_cast<unsigned int>(
		n % 1 == 0 && n % 10 == 1 && n % 100 != 11														  ? 0
		: n % 1 == 0 && n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14)					  ? 1
		: n % 1 == 0 && (n % 10 == 0 || (n % 10 >= 5 && n % 10 <= 9) || (n % 100 >= 11 && n % 100 <= 14)) ? 2
																										  : 3);
}
unsigned int plural5_ga(int n) {
	return static_cast<unsigned int>(n == 1 ? 0 : n == 2 ? 1 : n < 7 ? 2 : n < 11 ? 3 : 4);
}
unsigned int plural6_ar(int n) {
	return static_cast<unsigned int>(n == 0							 ? 0
									 : n == 1						 ? 1
									 : n == 2						 ? 2
									 : n % 100 >= 3 && n % 100 <= 10 ? 3
									 : n % 100 >= 11				 ? 4
																	 : 5);
}

} // namespace

PluralForms PluralForms::fromString(const core::String &str) {
	typedef core::DynamicStringMap<PluralForms, 11> PluralFormsMap;
	static PluralFormsMap plural_forms;

	if (plural_forms.empty()) {
		// Note that the plural forms here shouldn't contain any spaces
		plural_forms.put("Plural-Forms:nplurals=1;plural=0;", PluralForms(1, plural1));
		plural_forms.put("Plural-Forms:nplurals=2;plural=(n!=1);", PluralForms(2, plural2_1));
		plural_forms.put("Plural-Forms:nplurals=2;plural=n!=1;", PluralForms(2, plural2_1));
		plural_forms.put("Plural-Forms:nplurals=2;plural=(n>1);", PluralForms(2, plural2_2));
		plural_forms.put("Plural-Forms:nplurals=2;plural=n==1||n%10==1?0:1;", PluralForms(2, plural2_mk));
		plural_forms.put("Plural-Forms:nplurals=2;plural=(n%10==1&&n%100!=11)?0:1;", PluralForms(2, plural2_mk_2));
		plural_forms.put("Plural-Forms:nplurals=3;plural=n%10==1&&n%100!=11?0:n!=0?1:2);", PluralForms(2, plural3_lv));
		plural_forms.put("Plural-Forms:nplurals=3;plural=n==1?0:n==2?1:2;", PluralForms(3, plural3_ga));
		plural_forms.put("Plural-Forms:nplurals=3;plural=(n%10==1&&n%100!=11?0:n%10>=2&&(n%100<10||n%100>=20)?1:2);",
						 PluralForms(3, plural3_lt));
		plural_forms.put(
			"Plural-Forms:nplurals=3;plural=(n%10==1&&n%100!=11?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2);",
			PluralForms(3, plural3_1));
		plural_forms.put("Plural-Forms:nplurals=3;plural=(n==1)?0:(n>=2&&n<=4)?1:2;", PluralForms(3, plural3_sk));
		plural_forms.put("Plural-Forms:nplurals=3;plural=(n==1?0:n%10>=2&&n%10<=4&&(n%100<10||n%100>=20)?1:2);",
						 PluralForms(3, plural3_pl));
		plural_forms.put("Plural-Forms:nplurals=3;plural=(n%100==1?0:n%100==2?1:n%100==3||n%100==4?2:3);",
						 PluralForms(3, plural3_sl));
		plural_forms.put("Plural-Forms:nplurals=3;plural=(n==1?0:(((n%100>19)||((n%100==0)&&(n!=0)))?2:1));",
						 PluralForms(3, plural3_ro));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n%1==0&&n==1?0:n%1==0&&n>=2&&n<=4?1:n%1!=0?2:3);",
						 PluralForms(4, plural4_sk));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n==1&&n%1==0)?0:(n>=2&&n<=4&&n%1==0)?1:(n%1!=0)?2:3;",
						 PluralForms(4, plural4_cs));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n%10==1&&n%100!=11?0:n%10>=2&&n%10<=4&&(n%100<12||n%100>14)?"
						 "1:n%10==0||(n%10>=5&&n%10<=9)||(n%100>=11&&n%100<=14)?2:3);",
						 PluralForms(4, plural4_be));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n==1||n==11)?0:(n==2||n==12)?1:(n>2&&n<20)?2:3;",
						 PluralForms(4, plural4_gd));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n==1)?0:(n==2)?1:(n!=8&&n!=11)?2:3;",
						 PluralForms(4, plural4_cy));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n%10==1&&(n%100>19||n%100<11)?0:(n%10>=2&&n%10<=9)&&(n%100>"
						 "19||n%100<11)?1:n%1!=0?2:3);",
						 PluralForms(4, plural4_lt));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n%1==0&&n%10==1&&n%100!=11?0:n%1==0&&n%10>=2&&n%10<=4&&(n%"
						 "100<12||n%100>14)?1:n%1==0&&(n%10==0||(n%10>=5&&n%10<=9)||(n%100>=11&&n%100<=14))?2:3);",
						 PluralForms(4, plural4_uk));
		plural_forms.put("Plural-Forms:nplurals=4;plural=(n==1?0:(n%10>=2&&n%10<=4)&&(n%100<12||n%100>14)?1:n!=1&&(n%"
						 "10>=0&&n%10<=1)||(n%10>=5&&n%10<=9)||(n%100>=12&&n%100<=14)?2:3);",
						 PluralForms(4, plural4_pl));
		plural_forms.put(
			"Plural-Forms:nplurals=4;plural=(n==1&&n%1==0)?0:(n==2&&n%1==0)?1:(n%10==0&&n%1==0&&n>10)?2:3;",
			PluralForms(4, plural4_he));
		plural_forms.put("Plural-Forms:nplurals=5;plural=(n==1?0:n==2?1:n<7?2:n<11?3:4)", PluralForms(5, plural5_ga));
		plural_forms.put("Plural-Forms:nplurals=6;plural=n==0?0:n==1?1:n==2?2:n%100>=3&&n%100<=10?3:n%100>=11?4:5",
						 PluralForms(6, plural6_ar));
	}

	// Remove spaces from string before lookup
	core::String space_less_str;
	for (size_t i = 0; i < str.size(); ++i)
		if (!core::string::isspace(str[i]))
			space_less_str += str[i];

	auto it = plural_forms.find(space_less_str);
	if (it != plural_forms.end()) {
		return it->second;
	}
	return PluralForms();
}

} // namespace app
