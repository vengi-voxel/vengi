local gui = require "ui.shared"
local style = require "ui.style"

function main()
	gui.windowTitle('Main', 800, 300, function ()
		gui.row(1)
		gui.windowPushButton('Connect', 'login')
		gui.row(1)
		gui.windowPushButton('Options', 'options')
		gui.spacing(1)
		gui.row(1)
		gui.windowPushCommand('Quit', 'quit')
	end, 'border')
end
