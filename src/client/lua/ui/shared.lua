local module = {}

local style = require "ui.style"

--[[
]]
function module.pop()
	ui.windowPop()
end

--[[
]]
function module.windowTitle(title, width, height, closure, ...)
	if ui.windowBegin(title, title, width, height, table.unpack({...})) then
		closure()
	else
		module.pop()
	end
	ui.windowEnd()
end

--[[
]]
function module.window(id, width, height, closure, ...)
	if ui.windowBegin(id, width, height, table.unpack({...})) then
		closure()
	else
		module.pop()
	end
	ui.windowEnd()
end

--[[
]]
function module.group(title, closure, ...)
	if ui.groupBegin(title, table.unpack({...})) then
		closure()
		ui.groupEnd()
	end
end

--[[
]]
function module.windowPushButton(title, windowId)
	if ui.button(title) then
		ui.windowPush(windowId)
	end
end

--[[
]]
function module.windowPushCommand(title, command)
	if ui.button(title) then
		cmd.execute(command)
	end
end

--[[
]]
function module.button(title, closure)
	if ui.button(title) then
		closure()
	end
end

--[[
]]
function module.tooltip(message)
	ui.tooltip(message)
end

--[[
]]
function module.text(message, options)
	ui.text(message, options)
end

--[[
]]
function module.row(columns)
	ui.layoutRow('dynamic', style.rowHeight, columns)
end

--[[
]]
function module.spacing(value)
	ui.spacing(value)
end

--[[
]]
function module.varStr(displayname, varname)
	local edit = {value = var.str(varname)}
	module.row({0.4, 0.6})
	ui.label(displayname)
	local state, changed = ui.edit('simple', edit)
	if changed then
		var.setstr(varname, edit.value)
	end
end

--[[
]]
function module.varCheckbox(displayname, varname)
	local edit = {value = var.bool(varname)}
	module.row({0.4, 0.6})
	ui.label(displayname)
	if ui.checkbox(displayname, edit) then
		var.setbool(varname, edit.value)
	end
end

--[[
]]
function module.varSlider(displayname, varname, min, max, step)
	local edit = {value = var.float(varname)}
	module.row({0.4, 0.6})
	ui.label(displayname)
	if ui.slider(min, edit, max, step) then
		var.setfloat(varname, edit.value)
	end
end

--[[
	Adds a generic back button that should always look the same
]]
function module.back()
	ui.layoutRow('dynamic', 30, 1)
	module.row(1)
	style.push({
		button = {
			["normal"] = "#668080",
			["active"] = "#99cccc",
			["hover"] = "#99cccc",
			["border color"] = "#809090"
		}
	})

	if ui.button('Back') then
		module.pop()
	end
	style.pop()
end

return module
