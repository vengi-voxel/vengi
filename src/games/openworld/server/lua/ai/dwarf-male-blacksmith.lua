local module = {}

local shared = require "ai.shared"

function module.register()
  local name = "DWARF_MALE_BLACKSMITH"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  shared.idlehome(rootNode)
end

return module
