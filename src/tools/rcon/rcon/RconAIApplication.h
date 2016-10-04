#pragma once

#include "AIApplication.h"
#include "RconAIDebugger.h"
#include "AIDebuggerWidget.h"

namespace rcon {

class RconAIApplication: public ai::debug::AIApplication {
private:
	using Super = ai::debug::AIApplication;
public:
	RconAIApplication(int argc, char** argv) :
			Super(argc, argv) {
	}

	void init() override {
		Super::init();
		const QList<QString>& args = QCoreApplication::arguments();
		if (args.size() != 3) {
			qDebug() << "connect to 127.0.0.1 on port 11338";
			_widget->connectToAIServer("127.0.0.1", 11338);
		}
	}

	ai::debug::AIDebugger* createDebugger() override {
		return new RconAIDebugger(*_resolver);
	}
};

}
