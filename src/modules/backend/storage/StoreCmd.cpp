/**
 * @file
 */

#include "StoreCmd.h"

namespace backend {

void StoreCmd::addComd() {
	core::Command::registerCommand("store", [&](const core::CmdArgs & args) {
		if (args.size() == 1) {
			if (args[0] == "help") {
				Log::info("store useradd <name> <password>\tadd a new users");
			} else if (args[0] == "init") {
				Persister pq;
				pq.init();
				pq.initTables();
			}
		} else if (args.size() == 3) {
			if (args[0] == "useradd") {
				const std::string tmUid = "0";
				const std::string& tmUser = args[1];
				const std::string& tmPw = args[2];
				Persister persister;
				persister.init();
				persister.storeUser(tmUser, tmPw, tmUid);
			}
		}
	});
}

}
