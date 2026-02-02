/**
 * @file
 */

#include "CVarsRequestHandler.h"
#include "core/Log.h"
#include "core/Var.h"
#include "voxedit-util/network/ServerNetwork.h"
#include "voxedit-util/network/protocol/CVarsListMessage.h"

namespace voxedit {

CVarsRequestHandler::CVarsRequestHandler(ServerNetwork *network) : _network(network) {
}

void CVarsRequestHandler::execute(const network::ClientId &clientId, CVarsRequestMessage *msg) {
	core::DynamicArray<CVarInfo> cvarInfos;
	core::Var::visit([&cvarInfos](const core::VarPtr &var) {
		CVarInfo info;
		info.name = var->name();
		if (var->getFlags() & core::CV_SECRET) {
			info.value = "***";
		} else {
			info.value = var->strVal();
		}
		const char *help = var->help();
		info.description = help != nullptr ? help : "";
		info.flags = var->getFlags();
		cvarInfos.push_back(info);
	});

	CVarsListMessage response(cvarInfos);
	if (!_network->sendToClient(clientId, response)) {
		Log::error("Failed to send cvars list to client %d", clientId);
	}
}

} // namespace voxedit
