require "attrib.shared"

function registerRabbit()
  local animal = attrib.createContainer("ANIMAL_RABBIT")
  animalDefault(animal)
  animal:addAbsolute("FIELDOFVIEW", 240.0)
end
