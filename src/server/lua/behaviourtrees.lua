require "ai.animal-rabbit"
require "ai.animal-wolf"
require "ai.dwarf-male-blacksmith"
require "ai.human-male-blacksmith"
require "ai.human-male-knight"
require "ai.human-male-shepherd"
require "ai.human-male-worker"
require "ai.undead-male-skeleton"
require "ai.undead-male-zombie"

function init ()
  registerDwarfBlacksmith()

  registerHumanBlacksmith()
  registerHumanKnight()
  registerHumanShepherd()
  registerHumanWorker()

  registerUndeadSkeleton()
  registerUndeadZombie()

  registerRabbit()
  registerWolf()
end
