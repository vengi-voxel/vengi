local module = {}

local shared = require "attrib.shared"

function module.register()
  local animal = attrib.createContainer("ANIMAL_RABBIT")
  shared.animalDefault(animal)
  animal:addAbsolute("FIELDOFVIEW", 240.0)
end

return module
