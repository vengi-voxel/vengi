require "ai.shared"

function registerSkeleton ()
  local name = "UNDEAD_MALE_SKELETON"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
