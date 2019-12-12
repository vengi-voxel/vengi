require "ai.shared"

function registerDwarfBlacksmith ()
  local name = "DWARF_MALE_BLACKSMITH"
  local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
  idlehome(rootNode)
end
