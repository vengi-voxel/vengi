function animalDefault(container)
	container:absolute("SPEED", 0.8)
	container:absolute("HEALTH", 100.0)
	container:absolute("ATTACKRANGE", 0.0)
	container:absolute("STRENGTH", 5.0)
end

function init()
	local wolf = attrib.createContainer("ANIMAL_WOLF")
	animalDefault(wolf)
	wolf:absolute("ATTACKRANGE", 2.0)
	wolf:register()

	local rabbit = attrib.createContainer("ANIMAL_RABBIT")
	animalDefault(rabbit)
	rabbit:absolute("SPEED", 2.0)
	rabbit:register()
end
