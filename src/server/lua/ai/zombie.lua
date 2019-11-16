require "ai.shared"

function registerZombie ()
	local name = "UNDEAD_MALE_DEFAULT"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	idle(rootNode)
end
