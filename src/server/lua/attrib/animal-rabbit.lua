require "attrib.shared"

function registerRabbit()
	local rabbit = attrib.createContainer("ANIMAL_RABBIT")
	animalDefault(rabbit)
	rabbit:absolute("FIELDOFVIEW", 240.0)
	rabbit:register()
end

