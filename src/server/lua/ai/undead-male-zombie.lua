require "ai.shared"

function registerUndeadZombie ()
  local name = "UNDEAD_MALE_ZOMBIE"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
