-- An overview of most of the supported widgets.
-- Simple calculator example

local ops = {'+', '-', '*', '/'}
local a, b, op = '0'

local function clear()
	a, b, op = '0'
end

local function digit(d)
	if op then
		if b == nil or b == '0' then
			b = d
		else
			b = b..d
		end
	else
		if a == '0' then
			a = d
		else
			a = a..d
		end
	end
end

local function decimal()
	if op then
		b = b or '0'
		b = b:find('.') and b or b..'.'
	else
		a = a:find('.') and a or a..'.'
	end
end

local function equals()
	if not tonumber(b) then
		return
	end
	if op == '+' then
		a, b, op = tostring(tonumber(a) + tonumber(b))
	elseif op == '-' then
		a, b, op = tostring(tonumber(a) - tonumber(b))
	elseif op == '*' then
		a, b, op = tostring(tonumber(a) * tonumber(b))
	elseif op == '/' then
		a, b, op = tostring(tonumber(a) / tonumber(b))
	end
end

local function operator(o)
	if op then
		equals()
	end
	op = o
end

local function display()
	return b or a
end

local checkA = {value = false}
local checkB = {value = true}
local radio = {value = 'A'}
local selectA = {value = false}
local selectB = {value = true}
local slider = {value = 0.2}
local progress = {value = 1}
local colorPicker = {value = '#ff0000'}
local property = {value = 6}
local edit = {value = 'Edit text'}
local comboA = {value = 1, items = {'A', 'B', 'C'}}
local modelScale = 1.0
local modelOmegaY = {value = 0.0}
local modelCameraPos = vec3.new(0, 50, 100);
local modelCameraTarget = vec3.new(0, 0, 0)

function overview()
	ui.styleDefault()
	if ui.windowBegin('Overview', 100, 100, 600, 450, 'border', 'movable', 'title') then
		ui.menubarBegin()
		ui.layoutRowBegin('static', 25, 5);
		ui.layoutRowPush(45);
		if ui.menuBegin('Menu', nil, 120, 90) then
			ui.layoutRow('dynamic', 40, 1)
			ui.menuItem('Item A')
			ui.menuItem('Item B')
			ui.menuItem('Item C')
			ui.menuEnd()
		end
		ui.menubarEnd()
		ui.layoutRow('dynamic', 400, 3)
		if ui.groupBegin('Group 1', 'border') then
			ui.layoutRow('dynamic', 30, 1)
			ui.label('Left label')
			ui.label('Centered label', 'centered')
			ui.label('Right label', 'right')
			ui.label('Colored label', 'left', '#ff0000')
			if ui.treePush('tab', 'Tree Tab') then
				if ui.treePush('node', 'Tree Node 1') then
					ui.label('Label 1')
					ui.treePop()
				end
				if ui.treePush('node', 'Tree Node 2') then
					ui.label('Label 2')
					ui.treePop()
				end
				ui.treePop()
			end
			ui.spacing(1)
			if ui.button('Button') then
				print('button pressed!')
			end
			ui.spacing(1)
			ui.checkbox('Checkbox A', checkA)
			ui.checkbox('Checkbox B', checkB)
			ui.groupEnd()
		end
		if ui.groupBegin('Group 2', 'border') then
			ui.layoutRow('dynamic', 30, 1)
			ui.label('Radio buttons:')
			ui.layoutRow('dynamic', 30, 3)
			ui.radio('A', radio)
			ui.radio('B', radio)
			ui.radio('C', radio)
			ui.layoutRow('dynamic', 30, 1)
			ui.selectable('Selectable A', selectA)
			ui.selectable('Selectable B', selectB)
			ui.layoutRow('dynamic', 30, {.35, .65})
			ui.label('Slider:')
			ui.slider(0, slider, 1, 0.05)
			ui.label('Progress:')
			ui.progress(progress, 10, true)
			ui.layoutRow('dynamic', 30, 2)
			ui.spacing(2)
			ui.label('Color picker:')
			ui.button(nil, colorPicker.value)
			ui.layoutRow('dynamic', 90, 1)
			ui.colorpicker(colorPicker)
			ui.groupEnd()
		end
		if ui.groupBegin('Group 3', 'border') then
			ui.layoutRow('dynamic', 30, 1)
			ui.property('Property', 0, property, 10, 0.25, 0.05)
			ui.spacing(1)
			ui.label('Edit:')
			ui.layoutRow('dynamic', 90, 1)
			ui.edit('box', edit)
			ui.layoutRow('dynamic', 5, 1)
			ui.spacing(1)
			ui.layoutRow('dynamic', 30, 1)
			ui.label('Combobox:')
			ui.combobox(comboA, comboA.items)
			ui.layoutRow('dynamic', 5, 1)
			ui.spacing(1)
			ui.layoutRow('dynamic', 30, 1)
			if ui.isWidgetHovered() then
				ui.tooltip('Test tooltip')
			end
			local x, y, w, h = ui.getWidgetBounds()
			if ui.contextualBegin(100, 100, x, y, w, h) then
				ui.layoutRow('dynamic', 30, 1)
				ui.contextualItem('Item A')
				ui.contextualItem('Item B')
				ui.contextualEnd()
			end
			ui.label('Contextual (Right click me)')
			ui.groupEnd()
		end
	end
	ui.windowEnd()
end

function modelwin()
	ui.styleDefault()
	if ui.windowBegin('Model', 600, 250, 1024, 768, 'border', 'movable', 'title') then
		ui.layoutRow('dynamic', 768, { 0.2, 0.8 })
		ui.groupBegin('modelsettings', 'border')
			ui.layoutRow('dynamic', 30, 1)
			modelScale = ui.property('Scale', 0, modelScale, 10, 0.25, 0.05)
			ui.property('Omega', 0, modelOmegaY, 10, 0.1, 0.01)
			ui.property('Camera pos', -1000, modelCameraPos, 1000, 1, 1)
			ui.property('Camera target', -1000, modelCameraTarget, 1000, 1, 1)
		ui.groupEnd()

		ui.groupBegin('model', 'border')
			ui.layoutRow('dynamic', 768, 1)
			local options = {
				scale = modelScale,
				omegaY = modelOmegaY.value,
				cameraPos = modelCameraPos,
				cameraTarget = modelCameraTarget
			}
			ui.model('assets/north-dir', options)
		ui.groupEnd()
	end
	ui.windowEnd()
end

function calculator()
	ui.styleDefault()
	if ui.windowBegin('Calculator', 50, 50, 180, 250, 'border', 'movable', 'title') then
		ui.layoutRow('dynamic', 35, 1)
		ui.label(display(), 'right')
		ui.layoutRow('dynamic', 35, 4)
		for i=1,16 do
			if i >= 13 and i < 16 then
				if i == 13 then
					if ui.button('C') then
						clear()
					end
					if ui.button('0') then
						digit('0')
					end
					if ui.button('=') then
						equals()
					end
				end
			elseif i % 4 ~= 0 then
				local d = tostring(math.floor(i / 4) * 3 + (i % 4))
				if ui.button(d) then
					digit(d)
				end
			else
				local o = ops[math.floor(i / 4)]
				if ui.button(o) then
					operator(o)
				end
			end
		end
	end
	ui.windowEnd()
end

local colors = {
	['text'] = '#afafaf',
	['window'] = '#2d2d2d',
	['header'] = '#282828',
	['border'] = '#414141',
	['button'] = '#323232',
	['button hover'] = '#282828',
	['button active'] = '#232323',
	['toggle'] = '#646464',
	['toggle hover'] = '#787878',
	['toggle cursor'] = '#2d2d2d',
	['select'] = '#2d2d2d',
	['select active'] = '#232323',
	['slider'] = '#262626',
	['slider cursor'] = '#646464',
	['slider cursor hover'] = '#787878',
	['slider cursor active'] = '#969696',
	['property'] = '#262626',
	['edit'] = '#262626',
	['edit cursor'] = '#afafaf',
	['combo'] = '#2d2d2d',
	['chart'] = '#787878',
	['chart color'] = '#2d2d2d',
	['chart color highlight'] = '#ff0000',
	['scrollbar'] = '#282828',
	['scrollbar cursor'] = '#646464',
	['scrollbar cursor hover'] = '#787878',
	['scrollbar cursor active'] = '#969696',
	['tab header'] = '#282828'
}

local colorNames = {}

for name,_ in pairs(colors) do
	colorNames[#colorNames + 1] = name
end

function stylewin()
	ui.styleLoadColors(colors)
	if ui.windowBegin('Style', 400, 50, 350, 450, 'border', 'movable', 'title', 'scrollbar') then
		ui.layoutRow('dynamic', 25, 2)
		for _,name in ipairs(colorNames) do
			ui.label(name..':')
			local color = colors[name]
			if ui.comboboxBegin(nil, color, 200, 200) then
				ui.layoutRow('dynamic', 90, 1)
				color = ui.colorpicker(color)
				colors[name] = color
				ui.comboboxEnd()
			end
		end
	end
	ui.windowEnd()
end
