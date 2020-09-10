local module = {}

local shared = require "attrib.shared"

function module.register()
  local chr = attrib.createContainer("UNDEAD_MALE_ZOMBIE")
  shared.characterDefault(chr)
  chr:addAbsolute("SPEED", 8.0)
end

return module
