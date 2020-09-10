local module = {}

local shared = require "attrib.shared"

function module.register()
  local animal = attrib.createContainer("ANIMAL_WOLF")
  shared.animalDefault(animal)
  animal:addAbsolute("VIEWDISTANCE", 500.0)
  animal:addAbsolute("FIELDOFVIEW", 240.0)
  animal:addAbsolute("ATTACKRANGE", 2.0)
  animal:addAbsolute("SPEED", 16.0)
end

return module
