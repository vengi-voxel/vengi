local module = {}

local shared = require "ai.shared"

function module.register()
  local name = "HUMAN_FEMALE_WORKER"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  shared.idlehome(rootNode)
end

return module
