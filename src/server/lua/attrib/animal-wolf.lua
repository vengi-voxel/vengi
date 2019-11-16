require "attrib.shared"

function registerWolf()
	local wolf = attrib.createContainer("ANIMAL_WOLF")
	animalDefault(wolf)
	wolf:absolute("VIEWDISTANCE", 500.0)
	wolf:absolute("FIELDOFVIEW", 240.0)
	wolf:absolute("ATTACKRANGE", 2.0)
	wolf:absolute("SPEED", 1.7)
	wolf:register()
end

