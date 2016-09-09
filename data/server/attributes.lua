function animalDefault(container)
	container:absolute("SPEED", 40.0)
	container:absolute("HEALTH", 100.0)
	container:absolute("ATTACKRANGE", 0.0)
	container:absolute("STRENGTH", 5.0)
	container:absolute("VIEWDISTANCE", 50.0)
end

function init()
	local wolf = attrib.createContainer("ANIMAL_WOLF")
	animalDefault(wolf)
	wolf:absolute("VIEWDISTANCE", 100.0)
	wolf:absolute("ATTACKRANGE", 2.0)
	wolf:register()

	local rabbit = attrib.createContainer("ANIMAL_RABBIT")
	animalDefault(rabbit)
	rabbit:absolute("SPEED", 60.0)
	rabbit:register()

	local player = attrib.createContainer("PLAYER")
	player:absolute("SPEED", 35.0)
	player:absolute("VIEWDISTANCE", 500.0)
	player:register()
end
