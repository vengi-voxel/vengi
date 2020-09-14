local style = require "ui.style"
local gui = require "ui.shared"

local colorNames = {}
for key,_ in pairs(style.colors) do
	colorNames[#colorNames + 1] = key
end

local styleNames = {}
for key,_ in pairs(style.style) do
	styleNames[#styleNames + 1] = key
end

function style()
	if ui.windowBegin('Colors and ui settings', 400, 50, 800, 800, 'border', 'movable', 'title', 'scrollbar', 'scalable') then
		ui.layoutRow('dynamic', 400, 1)
		gui.group('Colors', function()
			ui.layoutRow('dynamic', 25, 2)
			for _,name in ipairs(colorNames) do
				ui.label(name..':')
				local color = style.colors[name]
				if ui.comboboxBegin(nil, color, 200, 200) then
					ui.layoutRow('dynamic', 90, 1)
					local edit = { value = color };
					local changed = ui.colorpicker(edit)
					ui.comboboxEnd()
					if changed then
						style.colors[name] = edit.value
						style.shutdown()
						style.init()
					end
				end
			end
		end, 'border', 'title', 'scrollbar', 'scroll_auto_hide')

		ui.layoutRow('dynamic', 400, 1)
		gui.group('Style', function()
			for _,name in ipairs(styleNames) do
				ui.label(name..':')
			end
		end, 'border', 'title', 'scrollbar', 'scroll_auto_hide')
	end
	ui.windowEnd()
end
