require "attrib.shared"

function registerRabbit()
  local animal = attrib.createContainer("ANIMAL_RABBIT")
  animalDefault(animal)
  animal:absolute("FIELDOFVIEW", 240.0)
  animal:register()
end
