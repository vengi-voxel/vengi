require "ai.shared"

function registerHumanShepherd ()
  local name = "HUMAN_MALE_SHEPHERD"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
