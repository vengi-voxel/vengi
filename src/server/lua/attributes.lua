require "attrib.animal-rabbit"
require "attrib.animal-wolf"
require "attrib.dwarf-male-blacksmith"
require "attrib.human-male-blacksmith"
require "attrib.human-male-knight"
require "attrib.human-male-shepherd"
require "attrib.human-male-worker"
require "attrib.undead-male-skeleton"
require "attrib.undead-male-zombie"

function init()
  registerDwarfBlacksmith()

  registerHumanBlacksmith()
  registerHumanKnight()
  registerHumanShepherd()
  registerHumanWorker()

  registerUndeadSkeleton()
  registerUndeadZombie()

  registerRabbit()
  registerWolf()

  local player = attrib.createContainer("PLAYER")
  player:absolute("SPEED", 20.0)
  player:absolute("HEALTH", 100.0)
  player:absolute("ATTACKRANGE", 1.0)
  player:absolute("STRENGTH", 1.0)
  player:absolute("VIEWDISTANCE", 500.0)
  player:register()
end
