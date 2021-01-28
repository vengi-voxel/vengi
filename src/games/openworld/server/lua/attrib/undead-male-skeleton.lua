local module = {}

local shared = require "attrib.shared"

function module.register()
  local chr = attrib.createContainer("UNDEAD_MALE_SKELETON")
  shared.characterDefault(chr)
end

return module
