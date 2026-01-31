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
	command::Command::registerCommand("varclearhistory", [] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: varclearhistory <cvar>");
			return;
		}
		const core::VarPtr& st = core::Var::get(args[0]);
		if (st) {
			st->clearHistory();
		}
	}).setHelp(_("Clear the value history of a variable"));

	command::Command::registerCommand("void", [] (const command::CmdArgs& args) {
	}).setHelp(_("Just a no-operation command"));

	command::Command::registerCommand("url", [] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: url <http://my-url>");
			return;
		}
		app::App::openURL(args[0]);
	}).setHelp(_("Open the given url in a browser"));

	command::Command::registerCommand("echo", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info(" ");
		}
		const core::String& params = core::string::join(args.begin(), args.end(), " ");
		Log::info("%s", params.c_str());
	}).setHelp(_("Print the given arguments to the console (info log level)"));

	command::Command::registerCommand("exec", [] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: exec <file>");
			return;
		}
		const core::String& cmds = io::filesystem()->load(args[0]);
		if (cmds.empty()) {
			Log::warn("Could not load script '%s' - or file was empty.", args[0].c_str());
			return;
		}
		command::Command::execute(cmds);
	}).setHelp(_("Execute a file with script commands")).setArgumentCompleter(command::fileCompleter(io::filesystem(), "", "*.cfg"));

	command::Command::registerCommand("toggle", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: toggle <cvar>");
			return;
		}
		const core::VarPtr& var = core::Var::get(args[0]);
		if (!var) {
			Log::error("given var doesn't exist: %s", args[0].c_str());
			return;
		}
		var->toggleBool();
	}).setHelp(_("Toggle between true/false for a variable"));

	command::Command::registerCommand("inc", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: inc <cvar> [<delta:1.0>]");
			return;
		}
		const core::VarPtr& var = core::Var::get(args[0]);
		if (!var) {
			Log::error("given var doesn't exist: %s", args[0].c_str());
			return;
		}
		const float delta = args.size() > 1 ? core::string::toFloat(args[1]) : 1.0f;
		var->setVal(var->floatVal() + delta);
		Log::debug("Increase %s by %f", var->name().c_str(), delta);
	}).setHelp(_("Increase a cvar value by the given value (default: 1)"));

	command::Command::registerCommand("dec", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: dec <cvar> [<delta:1.0>]");
			return;
		}
		const core::VarPtr& var = core::Var::get(args[0]);
		if (!var) {
			Log::error("given var doesn't exist: %s", args[0].c_str());
			return;
		}
		const float delta = args.size() > 1 ? core::string::toFloat(args[1]) : 1.0f;
		var->setVal(var->floatVal() - delta);
		Log::debug("Decrease %s by %f", var->name().c_str(), delta);
	}).setHelp(_("Decrease a cvar value by the given value (default: 1)"));

	command::Command::registerCommand("show", [] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: show <cvar>");
			return;
		}
		const core::VarPtr& st = core::Var::get(args[0]);
		if (st) {
			Log::info(" -> %s ", st->strVal().c_str());
		} else {
			Log::info("Variable %s not found", args[0].c_str());
		}
	}).setHelp(_("Show the value of a variable"));

	command::Command::registerCommand("timemillis", [&] (const command::CmdArgs& args) {
		const uint64_t millis = timeProvider->tickNow();
		Log::info("%" PRId64, millis);
	}).setHelp(_("Print current milliseconds to console"));

	command::Command::registerCommand("logerror", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::error("%s", args[0].c_str());
	}).setHelp(_("Log given message as error"));

	command::Command::registerCommand("loginfo", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::info("%s", args[0].c_str());
	}).setHelp(_("Log given message as info"));

	command::Command::registerCommand("logdebug", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::debug("%s", args[0].c_str());
	}).setHelp(_("Log given message as debug"));

	command::Command::registerCommand("logwarn", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::warn("%s", args[0].c_str());
	}).setHelp(_("Log given message as warn"));

	command::Command::registerCommand("cvarjson", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: cvarjson <file>");
			return;
		}
		const core::String &filename = args[0];
		core::String json = "{\n";
		bool first = true;
		util::visitVarSorted([&] (const core::VarPtr& var) {
			if (!first) {
				json += ",\n";
			} else {
				first = false;
			}
			const uint32_t flags = var->getFlags();
			const char *value = (flags & core::CV_SECRET) ? "***secret***" : var->strVal().c_str();
			json += core::String::format("\"%s\": {", var->name().c_str());
			json += core::String::format("\"value\": \"%s\",", value);
			json += core::String::format("\"flags\": %u", flags);
			if (var->help()) {
				json += core::String::format(",\"help\": \"%s\"", var->help());
			}
			json += "}";
		}, 0u);
		json += "}";
		if (io::Filesystem::sysWrite(filename, json)) {
			Log::info("Wrote cvar json to %s", filename.c_str());
		} else {
			Log::error("Failed to write cvar json to %s", filename.c_str());
		}
	}).setHelp(_("Print the list of all known variables as json to a file"));

	command::Command::registerCommand("cvarlist", [] (const command::CmdArgs& args) {
		util::visitVarSorted([&] (const core::VarPtr& var) {
			if (!args.empty() && !core::string::matches(var->name(), args[0])) {
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
			const char *help = var->help();
			if (help != nullptr) {
				Log::info("        %s", help);
			}
		}, 0u);
	}).setHelp(_("Show the list of known variables (wildcards supported)"));

	command::Command::registerCommand("cmdlist", [] (const command::CmdArgs& args) {
		command::Command::visitSorted([&] (const command::Command& cmd) {
			if (!args.empty() && !core::string::matches(cmd.name(), args[0])) {
				return;
			}
			Log::info("* %s - %s", cmd.name().c_str(), cmd.help().c_str());
		});
	}).setHelp(_("Show the list of known commands (wildcards supported)"));

	command::Command::registerCommand("cmdjson", [] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: cmdjson <file>");
			return;
		}
		const core::String &filename = args[0];
		core::String json = "{\n";
		bool first = true;
		command::Command::visitSorted([&] (const command::Command& cmd) {
			if (!first) {
				json += ",\n";
			} else {
				first = false;
			}
			json += core::String::format("\"%s\": {", cmd.name().c_str());
			json += core::String::format("\"help\": \"%s\"", cmd.help().c_str());
			json += "}";
		});
		json += "}";
		if (io::Filesystem::sysWrite(filename, json)) {
			Log::info("Wrote cmd json to %s", filename.c_str());
		} else {
			Log::error("Failed to write cmd json to %s", filename.c_str());
		}
	}).setHelp(_("Print the list of all known commands as json to a file"));
}

}

}
