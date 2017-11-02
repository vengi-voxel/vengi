require "ai.shared"

function wolf ()
	local name = "BLACKSMITH"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	idle(rootNode)
end
