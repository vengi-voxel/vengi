local gui = require "ui.shared"
local style = require "ui.style"

local graphicHeight = (8 + style.rowHeight) * 8
function options()
	gui.windowTitle('GAME SETTINGS', 600, 800, function ()
		ui.layoutRow('dynamic', graphicHeight, 1)
		gui.group('Graphic', function()
			gui.varCheckbox('Fog', 'cl_fog')
			gui.varCheckbox('Shadow', 'cl_shadowmap')
			-- TODO: combobox with valid resolutions
			gui.varStr('Width', 'cl_width')
			gui.varStr('Height', 'cl_height')
			gui.varCheckbox('Fullscreen', 'cl_fullscreen')
			gui.varCheckbox('V-Sync', 'cl_vsync')
			gui.varSlider('Gamma', 'cl_gamma', 0.1, 4.0, 0.1)
		end, 'border', 'title', 'scrollbar', 'scroll_auto_hide')

		gui.group('Sound', function()
			gui.varSlider('Sound', 'snd_volume', 0, 255, 1)
			gui.varSlider('Music', 'snd_musicvolume', 0, 255, 1)
		end, 'border', 'title', 'scrollbar', 'scroll_auto_hide')

		gui.back()
	end, 'title')
end
