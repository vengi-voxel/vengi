/**
 * @file
 */

#include "ExecuteCommandHandler.h"
#include "command/CommandHandler.h"

namespace backend {

void ExecuteCommandHandler::executeWithRaw(void* attachment, const ai::ExecuteCommand* message, const uint8_t* rawData, size_t rawDataSize) {
	command::executeCommands(message->command()->c_str());
}

}
