/**
 * @file
 */

#include "TestTurbobadger.h"
#include "testcore/TestAppMain.h"
#include "ui/turbobadger/TurboBadger.h"
#include "Demo.h"
#include "ListWindow.h"
#include "ResourceEditWindow.h"

extern AdvancedItemSource advanced_source;
extern tb::TBGenericStringItemSource name_source;
extern tb::TBGenericStringItemSource popup_menu_source;

const char *girl_names[] = {
	"Maja", "Alice", "Julia", "Linnéa", "Wilma", "Ella", "Elsa", "Emma", "Alva", "Olivia", "Molly", "Ebba", "Klara", "Nellie", "Agnes",
	"Isabelle", "Ida", "Elin", "Ellen", "Moa", "Emilia", "Nova", "Alma", "Saga", "Amanda", "Isabella", "Lilly", "Alicia", "Astrid",
	"Matilda", "Tuva", "Tilde", "Stella", "Felicia", "Elvira", "Tyra", "Hanna", "Sara", "Vera", "Thea", "Freja", "Lova", "Selma",
	"Meja", "Signe", "Ester", "Lovisa", "Ellie", "Lea", "Tilda", "Tindra", "Sofia", "Nora", "Nathalie", "Leia", "Filippa", "Siri",
	"Emelie", "Inez", "Edith", "Stina", "Liv", "Lisa", "Linn", "Tove", "Emmy", "Livia", "Jasmine", "Evelina", "Cornelia", "Märta",
	"Svea", "Ingrid", "My", "Rebecca", "Joline", "Mira", "Ronja", "Hilda", "Melissa", "Anna", "Frida", "Maria", "Iris", "Josefine",
	"Elise", "Elina", "Greta", "Vilda", "Minna", "Lina", "Hedda", "Nicole", "Kajsa", "Majken", "Sofie", "Annie", "Juni", "Novalie", "Hedvig", nullptr };
const char *boy_names[] = {
	"Oscar", "William", "Lucas", "Elias", "Alexander", "Hugo", "Oliver", "Theo", "Liam", "Leo", "Viktor", "Erik", "Emil",
	"Isak", "Axel", "Filip", "Anton", "Gustav", "Edvin", "Vincent", "Arvid", "Albin", "Ludvig", "Melvin", "Noah", "Charlie", "Max",
	"Elliot", "Viggo", "Alvin", "Alfred", "Theodor", "Adam", "Olle", "Wilmer", "Benjamin", "Simon", "Nils", "Noel", "Jacob", "Leon",
	"Rasmus", "Kevin", "Linus", "Casper", "Gabriel", "Jonathan", "Milo", "Melker", "Felix", "Love", "Ville", "Sebastian", "Sixten",
	"Carl", "Malte", "Neo", "David", "Joel", "Adrian", "Valter", "Josef", "Jack", "Hampus", "Samuel", "Mohammed", "Alex", "Tim",
	"Daniel", "Vilgot", "Wilhelm", "Harry", "Milton", "Maximilian", "Robin", "Sigge", "Måns", "Eddie", "Elton", "Vidar", "Hjalmar",
	"Loke", "Elis", "August", "John", "Hannes", "Sam", "Frank", "Svante", "Marcus", "Mio", "Otto", "Ali", "Johannes", "Fabian",
	"Ebbe", "Aron", "Julian", "Elvin", "Ivar", nullptr };

TestTurbobadger::TestTurbobadger(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testturbobadger");
}

core::AppState TestTurbobadger::onInit() {
	_applicationSkin = "ui/skin/skin.tb.txt";
	core::AppState state = Super::onInit();

	// Load language file
	if (!tb::g_tb_lng->load("demo01/language/lng_en.tb.txt")) {
		Log::warn("Could not load translation lng_en.tb.txt");
	}

	// Block new animations during Init.
	tb::TBAnimationBlocker anim_blocker;

	// TBSelectList and TBSelectDropdown widgets have a default item source that are fed with any items
	// specified in the resource files. But it is also possible to set any source which can save memory
	// and improve performance. Then you don't have to populate each instance with its own set of items,
	// for widgets that occur many times in a UI, always with the same items.
	// Here we prepare the name source, that is used in a few places.
	for (int i = 0; boy_names[i]; i++)
		advanced_source.addItem(new AdvancedItem(boy_names[i++], TBIDC("boy_item"), true));
	for (int i = 0; girl_names[i]; i++)
		advanced_source.addItem(new AdvancedItem(girl_names[i++], TBIDC("girl_item"), false));
	for (int i = 0; girl_names[i]; i++)
		name_source.addItem(new TBGenericStringItem(girl_names[i++], TBIDC("girl_item")));
	for (int i = 0; boy_names[i]; i++)
		name_source.addItem(new TBGenericStringItem(boy_names[i++], TBIDC("boy_item")));
	advanced_source.setSort(TB_SORT_ASCENDING);
	name_source.setSort(TB_SORT_ASCENDING);

	// Prepare a source with submenus (with eternal recursion) so we can test sub menu support.
	popup_menu_source.addItem(new TBGenericStringItem("Option 1", TBIDC("opt 1")));
	popup_menu_source.addItem(new TBGenericStringItem("Option 2", TBIDC("opt 2")));
	popup_menu_source.addItem(new TBGenericStringItem("-"));
	popup_menu_source.addItem(new TBGenericStringItem("Same submenu", &popup_menu_source));
	popup_menu_source.addItem(new TBGenericStringItem("Long submenu", &name_source));
	// Give the first item a skin image
	popup_menu_source.getItem(0)->setSkinImage(TBIDC("Icon16"));

	new MainWindow(_root);

	new EditWindow(_root);

	new ListWindow(_root, &name_source);

	new AdvancedListWindow(_root, &advanced_source);

	new TabContainerWindow(_root);

	return state;
}

TEST_APP(TestTurbobadger)
