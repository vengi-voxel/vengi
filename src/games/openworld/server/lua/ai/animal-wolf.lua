local module = {}

local shared = require "ai.shared"

local function wolfStayAlive (parentnode)
end

function module.register()
  local name = "ANIMAL_WOLF"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  wolfStayAlive(rootNode)
  shared.hunt(rootNode)
  shared.increasePopulation(rootNode)
  shared.idle(rootNode)
  shared.die(rootNode)
end

return module
