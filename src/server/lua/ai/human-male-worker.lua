require "ai.shared"

function registerWorker ()
	local name = "HUMAN_MALE_WORKER"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	idlehome(rootNode)
end
