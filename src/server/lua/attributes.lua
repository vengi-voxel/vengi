require "attrib.animal-rabbit"
require "attrib.animal-wolf"
require "attrib.dwarf-male-blacksmith"
require "attrib.human-male-blacksmith"
require "attrib.human-male-knight"
require "attrib.human-male-shepherd"
require "attrib.human-male-worker"
require "attrib.human-female-worker"
require "attrib.undead-male-skeleton"
require "attrib.undead-male-zombie"

function init()
  registerDwarfBlacksmith()

  registerHumanBlacksmith()
  registerHumanKnight()
  registerHumanShepherd()
  registerHumanMaleWorker()
  registerHumanFemaleWorker()

  registerUndeadSkeleton()
  registerUndeadZombie()

  registerRabbit()
  registerWolf()

  local player = attrib.createContainer("PLAYER")
  player:addAbsolute("SPEED", 20.0)
  player:addAbsolute("HEALTH", 100.0)
  player:addAbsolute("ATTACKRANGE", 1.0)
  player:addAbsolute("STRENGTH", 1.0)
  player:addAbsolute("VIEWDISTANCE", 500.0)
end
