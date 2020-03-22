local gui = require "ui.shared"

function options()
	gui.windowTitle('GAME SETTINGS', 600, 800, function ()
		ui.layoutRow('dynamic', 300, 1)
		gui.group('Graphic', function()
			gui.varCheckbox('Fog', 'cl_fog')
			gui.varStr('Height', 'cl_height')
			gui.varStr('Width', 'cl_width')
			gui.varCheckbox('Fullscreen', 'cl_fullscreen')
			gui.varSlider('Gamma', 'cl_gamma', 0.1, 4.0, 0.1)
		end, 'border', 'title')
		gui.back()
	end, 'title', 'closable')
end
