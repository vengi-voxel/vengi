local module = {}

local shared = require "attrib.shared"

function module.register()
  local chr = attrib.createContainer("HUMAN_MALE_BLACKSMITH")
  shared.characterDefault(chr)
end

return module
