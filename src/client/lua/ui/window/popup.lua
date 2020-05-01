local gui = require "ui.shared"

function popup(message)
	if message == nil then
		message = "No message given"
	end
	gui.window('popup', 480, 220, 400, 200, function ()
		gui.row(1)
		gui.text(message, {color = "#ff0000", size = 28, align = "center"})
		gui.back()
	end, 'border')
end
