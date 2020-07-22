function animalDefault(container)
	container:addAbsolute("SPEED", 1.5)
	container:addAbsolute("HEALTH", 100.0)
	container:addAbsolute("ATTACKRANGE", 0.0)
	container:addAbsolute("STRENGTH", 5.0)
	container:addAbsolute("VIEWDISTANCE", 50.0)
	container:addAbsolute("FIELDOFVIEW", 120.0)
end

function init()
	local wolf = attrib.createContainer("ANIMAL_WOLF")
	animalDefault(wolf)
	wolf:addAbsolute("VIEWDISTANCE", 500.0)
	wolf:addAbsolute("FIELDOFVIEW", 240.0)
	wolf:addAbsolute("ATTACKRANGE", 2.0)
	wolf:addAbsolute("SPEED", 1.7)

	local rabbit = attrib.createContainer("ANIMAL_RABBIT")
	animalDefault(rabbit)
	rabbit:addAbsolute("FIELDOFVIEW", 240.0)

	local player = attrib.createContainer("PLAYER")
	player:addAbsolute("SPEED", 1.0)
	player:addAbsolute("VIEWDISTANCE", 500.0)
end
