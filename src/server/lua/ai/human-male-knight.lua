require "ai.shared"

function registerKnight ()
	local name = "HUMAN_MALE_KNIGHT"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	idle(rootNode)
end
