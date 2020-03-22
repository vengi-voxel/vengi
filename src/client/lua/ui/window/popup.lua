local gui = require "ui.shared"

function popup(message)
	gui.window('popup', 480, 220, function ()
		gui.row(1)
		gui.text(message, {color = "#ff0000", size = 28, align = "center"})
		gui.back()
	end, 'border')
end
