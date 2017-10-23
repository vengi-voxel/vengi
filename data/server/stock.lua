function init()
	local i = stock.createItem(1, 'WEAPON')
	local s = i:shape()
	s:addRect(0, 0, 1, 1)

	local invMain = stock.createContainer("main")
	local invMainShape = invMain:shape()
	invMainShape:addRect(0, 0, 1, 1)
end

