require "attrib.shared"

function registerUndeadZombie()
  local chr = attrib.createContainer("UNDEAD_MALE_ZOMBIE")
  characterDefault(chr)
  chr:absolute("SPEED", 8.0)
  chr:register()
end
