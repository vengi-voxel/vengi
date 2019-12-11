require "attrib.animal-rabbit"
require "attrib.animal-wolf"
require "attrib.human-male-blacksmith"
require "attrib.human-male-knight"
require "attrib.human-male-worker"
require "attrib.undead-male-skeleton"
require "attrib.undead-male-zombie"

function init()
  registerBlacksmith()
  registerKnight()
  registerRabbit()
  registerSkeleton()
  registerWolf()
  registerWorker()
  registerZombie()

  local player = attrib.createContainer("PLAYER")
  player:absolute("SPEED", 20.0)
  player:absolute("HEALTH", 100.0)
  player:absolute("ATTACKRANGE", 1.0)
  player:absolute("STRENGTH", 1.0)
  player:absolute("VIEWDISTANCE", 500.0)
  player:register()
end
