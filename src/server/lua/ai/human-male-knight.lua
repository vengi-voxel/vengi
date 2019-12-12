require "ai.shared"

function registerHumanKnight ()
  local name = "HUMAN_MALE_KNIGHT"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
