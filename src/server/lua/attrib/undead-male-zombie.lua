require "attrib.shared"

function registerZombie()
  local chr = attrib.createContainer("UNDEAD_MALE_ZOMBIE")
  characterDefault(chr)
  chr:absolute("SPEED", 8.0)
  chr:register()
end
