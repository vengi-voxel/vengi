require "ai.shared"

function registerBlacksmith ()
  local name = "HUMAN_MALE_BLACKSMITH"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
