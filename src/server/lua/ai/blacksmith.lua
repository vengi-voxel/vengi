require "ai.shared"

function blacksmith ()
	local name = "BLACKSMITH"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	idle(rootNode)
end
