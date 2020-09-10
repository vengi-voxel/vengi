local module = {}

local shared = require "ai.shared"

local function rabbitStayAlive (parentnode)
  parentnode:addNode("Steer(SelectionFlee)", "fleefromhunter"):setCondition("And(Filter(SelectEntitiesOfTypes{ANIMAL_WOLF}),IsCloseToSelection{10})")
end

function module.register()
  local name = "ANIMAL_RABBIT"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  rabbitStayAlive(rootNode)
  shared.increasePopulation(rootNode)
  shared.idle(rootNode)
  shared.die(rootNode)
end

return module
