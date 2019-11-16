require "attrib.shared"

function registerWolf()
	local wolf = attrib.createContainer("ANIMAL_WOLF")
	animalDefault(wolf)
	wolf:absolute("VIEWDISTANCE", 500.0)
	wolf:absolute("FIELDOFVIEW", 240.0)
	wolf:absolute("ATTACKRANGE", 2.0)
	wolf:absolute("SPEED", 20.0)
	wolf:register()
end

