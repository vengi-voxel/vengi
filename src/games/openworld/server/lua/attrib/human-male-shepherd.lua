local module = {}

local shared = require "attrib.shared"

function module.register()
  local chr = attrib.createContainer("HUMAN_MALE_SHEPHERD")
  shared.characterDefault(chr)
end

return module
