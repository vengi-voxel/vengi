require "ai.shared"

function registerUndeadSkeleton ()
  local name = "UNDEAD_MALE_SKELETON"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
