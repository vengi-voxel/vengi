/**
 * @file
 */

#pragma once

#include <QApplication>
#include <QMainWindow>

namespace ai {
namespace debug {

class AIDebugger;
class AIDebuggerWidget;
class AINodeStaticResolver;

/**
 * @brief Use this class to run the debugger as a stand-alone application.
 *
 * @note If you embed the debugger into an already exsting QT application, you of course
 * don't need this.
 */
class AIApplication: public QApplication {
Q_OBJECT
protected:
	AIDebugger* _debugger;
	AINodeStaticResolver* _resolver;
	AIDebuggerWidget* _widget;
	QMainWindow _window;
public:
	AIApplication(int argc, char** argv);
	virtual ~AIApplication();
};

}
}
