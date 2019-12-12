require "attrib.shared"

function registerHumanBlacksmith()
  local chr = attrib.createContainer("HUMAN_MALE_BLACKSMITH")
  characterDefault(chr)
  chr:register()
end
