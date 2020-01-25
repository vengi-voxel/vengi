require "ai.shared"

function registerHumanFemaleWorker ()
  local name = "HUMAN_FEMALE_WORKER"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
