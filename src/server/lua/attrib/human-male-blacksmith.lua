require "attrib.shared"

function registerBlacksmith()
  local chr = attrib.createContainer("HUMAN_MALE_BLACKSMITH")
  characterDefault(chr)
  chr:register()
end
