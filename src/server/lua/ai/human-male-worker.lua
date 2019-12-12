require "ai.shared"

function registerHumanWorker ()
  local name = "HUMAN_MALE_WORKER"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
