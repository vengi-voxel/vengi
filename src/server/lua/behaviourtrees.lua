require "ai.animal-rabbit"
require "ai.animal-wolf"
require "ai.human-male-blacksmith"
require "ai.human-male-knight"
require "ai.human-male-worker"
require "ai.undead-male-skeleton"
require "ai.undead-male-zombie"

function init ()
  registerBlacksmith()
  registerKnight()
  registerRabbit()
  registerSkeleton()
  registerWolf()
  registerWorker()
  registerZombie()
end
