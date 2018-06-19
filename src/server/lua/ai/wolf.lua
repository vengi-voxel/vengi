require "ai.shared"

function wolfStayAlive (parentnode)
end

function wolf ()
	local name = "ANIMAL_WOLF"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	wolfStayAlive(rootNode)
	hunt(rootNode)
	increasePopulation(rootNode)
	idle(rootNode)
	die(rootNode)
end
