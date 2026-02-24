/**
 * @file
 */

#include "AppCommand.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "util/VarUtil.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/Var.h"
#include "app/I18N.h"

namespace app {

namespace AppCommand {

void init(const core::TimeProviderPtr& timeProvider) {
	command::Command::registerCommand("varclearhistory")
		.addArg({"cvar", command::ArgType::String, false, "", "Variable name"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &name = args.str("cvar");
			const core::VarPtr& st = core::findVar(name);
			if (st) {
				st->clearHistory();
			}
		}).setHelp(_("Clear the value history of a variable"));

	command::Command::registerCommand("void")
		.setHandler([] (const command::CommandArgs& args) {
		}).setHelp(_("Just a no-operation command"));

	command::Command::registerCommand("url")
		.addArg({"url", command::ArgType::String, false, "", "URL to open"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &url = args.str("url");
			app::App::openURL(url);
		}).setHelp(_("Open the given url in a browser"));

	command::Command::registerCommand("echo")
		.addArg({"message", command::ArgType::String, true, "", "Message to print"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &message = args.str("message");
			if (message.empty()) {
				Log::info(" ");
			} else {
				Log::info("%s", message.c_str());
			}
		}).setHelp(_("Print the given arguments to the console (info log level)"));

	command::Command::registerCommand("exec")
		.addArg({"file", command::ArgType::String, false, "", "Script file to execute"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &file = args.str("file");
			const core::String& cmds = io::filesystem()->load(file);
			if (cmds.empty()) {
				Log::warn("Could not load script '%s' - or file was empty.", file.c_str());
				return;
			}
			command::Command::execute(cmds);
		}).setHelp(_("Execute a file with script commands")).setArgumentCompleter(command::fileCompleter(io::filesystem(), "", "*.cfg"));

	command::Command::registerCommand("toggle")
		.addArg({"cvar", command::ArgType::String, false, "", "Variable name"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &cvar = args.str("cvar");
			const core::VarPtr& var = core::findVar(cvar);
			if (!var) {
				Log::error("given var doesn't exist: %s", cvar.c_str());
				return;
			}
			var->toggleBool();
		}).setHelp(_("Toggle between true/false for a variable"));

	command::Command::registerCommand("inc")
		.addArg({"cvar", command::ArgType::String, false, "", "Variable name"})
		.addArg({"delta", command::ArgType::Float, true, "1.0", "Amount to increase"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &cvar = args.str("cvar");
			const core::VarPtr& var = core::findVar(cvar);
			if (!var) {
				Log::error("given var doesn't exist: %s", cvar.c_str());
				return;
			}
			const float delta = args.floatVal("delta", 1.0f);
			var->setVal(var->floatVal() + delta);
			Log::debug("Increase %s by %f", var->name().c_str(), delta);
		}).setHelp(_("Increase a cvar value by the given value (default: 1)"));

	command::Command::registerCommand("dec")
		.addArg({"cvar", command::ArgType::String, false, "", "Variable name"})
		.addArg({"delta", command::ArgType::Float, true, "1.0", "Amount to decrease"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &cvar = args.str("cvar");
			const core::VarPtr& var = core::findVar(cvar);
			if (!var) {
				Log::error("given var doesn't exist: %s", cvar.c_str());
				return;
			}
			const float delta = args.floatVal("delta", 1.0f);
			var->setVal(var->floatVal() - delta);
			Log::debug("Decrease %s by %f", var->name().c_str(), delta);
		}).setHelp(_("Decrease a cvar value by the given value (default: 1)"));

	command::Command::registerCommand("show")
		.addArg({"cvar", command::ArgType::String, false, "", "Variable name"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &cvar = args.str("cvar");
			const core::VarPtr& st = core::findVar(cvar);
			if (st) {
				Log::info(" -> %s ", st->strVal().c_str());
			} else {
				Log::info("Variable %s not found", cvar.c_str());
			}
		}).setHelp(_("Show the value of a variable"));

	command::Command::registerCommand("timemillis")
		.setHandler([&] (const command::CommandArgs& args) {
			const uint64_t millis = timeProvider->tickNow();
			Log::info("%" PRId64, millis);
		}).setHelp(_("Print current milliseconds to console"));

	command::Command::registerCommand("logerror")
		.addArg({"message", command::ArgType::String, true, "", "Message to log"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &message = args.str("message");
			if (!message.empty()) {
				Log::error("%s", message.c_str());
			}
		}).setHelp(_("Log given message as error"));

	command::Command::registerCommand("loginfo")
		.addArg({"message", command::ArgType::String, true, "", "Message to log"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &message = args.str("message");
			if (!message.empty()) {
				Log::info("%s", message.c_str());
			}
		}).setHelp(_("Log given message as info"));

	command::Command::registerCommand("logdebug")
		.addArg({"message", command::ArgType::String, true, "", "Message to log"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &message = args.str("message");
			if (!message.empty()) {
				Log::debug("%s", message.c_str());
			}
		}).setHelp(_("Log given message as debug"));

	command::Command::registerCommand("logwarn")
		.addArg({"message", command::ArgType::String, true, "", "Message to log"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &message = args.str("message");
			if (!message.empty()) {
				Log::warn("%s", message.c_str());
			}
		}).setHelp(_("Log given message as warn"));

	command::Command::registerCommand("cvarlist")
		.addArg({"filter", command::ArgType::String, true, "", "Filter pattern (wildcards supported)"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &filter = args.str("filter");
			util::visitVarSorted([&] (const core::VarPtr& var) {
				if (!filter.empty() && !core::string::matches(var->name(), filter)) {
					return;
				}
				const uint32_t flags = var->getFlags();
				core::String flagsStr = "     ";
				const char *value = var->strVal().c_str();
				if ((flags & core::CV_READONLY) != 0) {
					flagsStr[0]  = 'R';
				}
				if ((flags & core::CV_NOPERSIST) != 0) {
					flagsStr[1]  = 'N';
				}
				if ((flags & core::CV_SHADER) != 0) {
					flagsStr[2]  = 'S';
				}
				if ((flags & core::CV_SECRET) != 0) {
					flagsStr[3]  = 'X';
					value = "***secret***";
				}
				if (var->isDirty()) {
					flagsStr[4]  = 'D';
				}
				Log::info("* %s %-28s = (%s) (%u)", flagsStr.c_str(), var->name().c_str(), value, var->getHistorySize());
				if (!var->description().empty()) {
					Log::info("        %s", var->description().c_str());
				}
			}, 0u);
		}).setHelp(_("Show the list of known variables (wildcards supported)"));

	command::Command::registerCommand("cmdlist")
		.addArg({"filter", command::ArgType::String, true, "", "Filter pattern (wildcards supported)"})
		.setHandler([] (const command::CommandArgs& args) {
			const core::String &filter = args.str("filter");
			command::Command::visitSorted([&] (const command::Command& cmd) {
				if (!filter.empty() && !core::string::matches(cmd.name(), filter)) {
					return;
				}
				Log::info("* %s - %s", cmd.name().c_str(), cmd.help().c_str());
			});
		}).setHelp(_("Show the list of known commands (wildcards supported)"));
}

}

}
