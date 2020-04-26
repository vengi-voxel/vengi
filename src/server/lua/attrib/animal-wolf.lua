require "attrib.shared"

function registerWolf()
  local animal = attrib.createContainer("ANIMAL_WOLF")
  animalDefault(animal)
  animal:absolute("VIEWDISTANCE", 500.0)
  animal:absolute("FIELDOFVIEW", 240.0)
  animal:absolute("ATTACKRANGE", 2.0)
  animal:absolute("SPEED", 16.0)
  animal:register()
end
