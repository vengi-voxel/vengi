/**
 * @file
 */

#include "app/App.h"
#include "app/benchmark/AbstractBenchmark.h"
#include "app/i18n/DictionaryManager.h"
#include "app/i18n/POParser.h"
#include "io/MemoryReadStream.h"

namespace app {

class DictionaryBenchmark : public AbstractBenchmark {};

BENCHMARK_F(DictionaryBenchmark, POParser)(benchmark::State &state) {
	const core::String poContent = R"(
msgid ""
msgstr ""
"Project-Id-Version: vengi\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: 2021-10-10 10:10+0200\n"
"PO-Revision-Date: 2021-10-10 10:10+0200\n"
"Last-Translator: \n"
"Language-Team: \n"
"Language: en\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n != 1);\n"

msgid "Hello World"
msgstr "Hallo Welt"

msgid "Another String"
msgstr "Ein anderer String"
)";

	for (auto _ : state) {
		io::MemoryReadStream stream(poContent.c_str(), poContent.size());
		Dictionary dict;
		POParser::parse("test.po", stream, dict);
	}
}

BENCHMARK_F(DictionaryBenchmark, DictionaryManagerLookup)(benchmark::State &state) {
	io::FilesystemPtr filesystem = app::App::getInstance()->filesystem();
	filesystem->homeWrite("locales/de.po", R"(
msgid "Hello World"
msgstr "Hallo Welt"
)");
	DictionaryManager mgr(filesystem);
	mgr.addDirectory(filesystem->homePath() + "locales");
	mgr.setLanguage(Language::fromSpec("de"));

	for (auto _ : state) {
		mgr.getDictionary().translate("Hello World");
	}
}

} // namespace app

BENCHMARK_MAIN();
