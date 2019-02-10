/**
 * @file
 */
#include "AIApplication.h"
#include "AIDebugger.h"
#include "AINodeStaticResolver.h"
#include "AIDebuggerWidget.h"
#include "Version.h"

#include <QList>
#include <QString>
#include <QToolBar>
#include <QMenuBar>
#include <QTranslator>

namespace ai {
namespace debug {

AIApplication::AIApplication(int argc, char** argv) :
		QApplication(argc, argv) {
}

void AIApplication::init() {
#ifdef Q_WS_X11
	QApplication::setGraphicsSystem(QLatin1String("raster"));
#endif
	setOrganizationName("engine");
	setOrganizationDomain("engine");
	setApplicationName("rcon");
	setApplicationVersion(VERSION);
#ifdef Q_WS_MAC
	a.setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

	_resolver = new AINodeStaticResolver();
	_debugger = createDebugger();
	_widget = new AIDebuggerWidget(*_debugger, *_resolver, true);

	_window.setCentralWidget(_widget);

	QToolBar* _toolbar = _window.addToolBar("");
	_toolbar->setMovable(false);
	_toolbar->setFloatable(false);
	_window.addToolBar(Qt::TopToolBarArea, _toolbar);

	_widget->contributeToStatusBar(_window.statusBar());
	_widget->contributeToToolBar(_toolbar);
	QMenu* menuBar = _window.menuBar()->addMenu(tr("&File"));
	_widget->contributeToFileMenu(menuBar);
	_widget->contributeToHelpMenu(_window.menuBar()->addMenu(tr("&Help")));
	_widget->contributeToSettingsMenu(_window.menuBar()->addMenu(tr("Settings")));
	_window.showMaximized();
	_window.show();

	const QList<QString>& args = QCoreApplication::arguments();
	if (args.size() == 3) {
		const QString& hostname = args.at(1);
		const short port = args.at(2).toShort();
		qDebug() << "connect to " << hostname << " on port " << port;
		_widget->connectToAIServer(hostname, port);
	}

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(),
			QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	installTranslator(&qtTranslator);

	QTranslator simpleaiTranslator;
	simpleaiTranslator.load(applicationName() + "_" + QLocale::system().name(), ":/data/");
	installTranslator(&simpleaiTranslator);
}

AIApplication::~AIApplication() {
	delete _debugger;
	delete _resolver;
	delete _widget;
}

AIDebugger* AIApplication::createDebugger() {
	return new AIDebugger(*_resolver);
}

}
}
