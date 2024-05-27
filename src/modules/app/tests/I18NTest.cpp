/**
 * @file
 */

#include "app/i18n/findlocale.h"
#include "app/tests/AbstractTest.h"

namespace app {

class I18NTest : public app::AbstractTest {};

TEST_F(I18NTest, testLocale) {
	Language systemLanguage;
	FL_Locale *locale = nullptr;
	if (FL_FindLocale(&locale, FL_MESSAGES) != FL_FAILED) {
		core::String lang = locale->lang ? locale->lang : "";
		core::String country = locale->country ? locale->country : "";
		core::String variant = locale->variant ? locale->variant : "";
		systemLanguage = Language::fromSpec(lang, country, variant);
	}
	if (!systemLanguage) {
		systemLanguage = Language::fromSpec("en", "GB");
	}
	FL_FreeLocale(&locale);
	ASSERT_TRUE(systemLanguage);
}

} // namespace app
