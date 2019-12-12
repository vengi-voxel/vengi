require "attrib.shared"

function registerHumanKnight()
  local chr = attrib.createContainer("HUMAN_MALE_KNIGHT")
  characterDefault(chr)
  chr:register()
end
