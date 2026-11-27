/**
 * @file
 */

#include "app/i18n/POParser.h"
#include "app/i18n/DictionaryManager.h"
#include "app/tests/AbstractTest.h"
#include "core/Path.h"
#include "io/FileStream.h"
#include "io/MemoryReadStream.h"

namespace app {

class POParserTest : public app::AbstractTest {
protected:
	const char *poString = R"(
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\n"
"PO-Revision-Date: 2024-04-12 13:50+0200\n"
"Last-Translator: Martin Gerhardy <someone@nowhere.com>\n"
"Language-Team: German <translation-team-de@lists.sourceforge.net>\n"
"Language: de\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=ASCII\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n != 1);\n"

msgid ""
"Multiline string "
"with no other meaning."
msgstr "Just a translation"

msgid "Single line string"
msgstr "Translation for Single line string"

#~ msgid "OK##treegenerate"
#~ msgstr "OK##baumgenerieren"

#~ msgid "Save As"
#~ msgstr "Speichern unter"

#~ msgid "Cut##scripteditor"
#~ msgstr "Ausschneiden##skripteditor"
)";
};

TEST_F(POParserTest, testParse) {
	const core::String filename = "mem";
	io::MemoryReadStream stream(poString, strlen(poString));
	Dictionary dict;
	ASSERT_TRUE(POParser::parse(filename, stream, dict));
	EXPECT_STREQ(dict.translate("Single line string"), "Translation for Single line string");
	EXPECT_STREQ(dict.translate("Multiline string with no other meaning."), "Just a translation");
}

TEST_F(POParserTest, testParseJaJP) {
	{
		const io::FilePtr &file = _testApp->filesystem()->open("po/ja_JP.po");
		ASSERT_TRUE(file) << "Failed to open po/ja_JP.po";
		io::FileStream stream(file);
		Dictionary dict;
		ASSERT_TRUE(POParser::parse(file->name(), stream, dict));
		// Dictionary::size() counts unique msgid entries plus unique msgctxt buckets
		EXPECT_GE(dict.size(), 1750u);
		const char *translated = dict.translate("Language");
		ASSERT_NE(translated, nullptr);
		EXPECT_STRNE(translated, "");
		EXPECT_STRNE(translated, "Language");
	}

	DictionaryManager mgr(_testApp->filesystem());
	for (const core::Path &path : _testApp->filesystem()->registeredPaths()) {
		mgr.addDirectory(path.str());
		mgr.addDirectory(path.append("po").str());
	}
	const Languages &langs = mgr.getLanguages();
	const Language jaLang = Language::fromSpec("ja", "JP");
	bool foundJa = false;
	for (const Language &lang : langs) {
		if (lang == jaLang) {
			foundJa = true;
			break;
		}
	}
	EXPECT_TRUE(foundJa);
	Dictionary &jaDict = mgr.getDictionary(jaLang);
	EXPECT_GE(jaDict.size(), 1750u);
	EXPECT_STRNE(jaDict.translate("Language"), "Language");
}

} // namespace app
