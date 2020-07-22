require "attrib.shared"

function registerUndeadZombie()
  local chr = attrib.createContainer("UNDEAD_MALE_ZOMBIE")
  characterDefault(chr)
  chr:addAbsolute("SPEED", 8.0)
end
