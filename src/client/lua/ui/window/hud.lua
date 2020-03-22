local gui = require "ui.shared"

function hud()
	-- todo transparent color background
	gui.window('hud', -1, -1, function ()
		gui.windowPushButton('Quit', 'quit')
	end)
end
