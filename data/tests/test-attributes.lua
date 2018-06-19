function animalDefault(container)
	container:absolute("SPEED", 1.5)
	container:absolute("HEALTH", 100.0)
	container:absolute("ATTACKRANGE", 0.0)
	container:absolute("STRENGTH", 5.0)
	container:absolute("VIEWDISTANCE", 50.0)
	container:absolute("FIELDOFVIEW", 120.0)
end

function init()
	local wolf = attrib.createContainer("ANIMAL_WOLF")
	animalDefault(wolf)
	wolf:absolute("VIEWDISTANCE", 500.0)
	wolf:absolute("FIELDOFVIEW", 240.0)
	wolf:absolute("ATTACKRANGE", 2.0)
	wolf:absolute("SPEED", 1.7)
	wolf:register()

	local rabbit = attrib.createContainer("ANIMAL_RABBIT")
	animalDefault(rabbit)
	rabbit:absolute("FIELDOFVIEW", 240.0)
	rabbit:register()

	local player = attrib.createContainer("PLAYER")
	player:absolute("SPEED", 1.0)
	player:absolute("VIEWDISTANCE", 500.0)
	player:register()
end
