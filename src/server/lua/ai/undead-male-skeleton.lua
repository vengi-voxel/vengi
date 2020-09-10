local module = {}

local shared = require "ai.shared"

function module.register()
  local name = "UNDEAD_MALE_SKELETON"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  shared.idlehome(rootNode)
end

return module
