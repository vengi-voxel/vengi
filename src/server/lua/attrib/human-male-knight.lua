require "attrib.shared"

function registerKnight()
  local chr = attrib.createContainer("HUMAN_MALE_KNIGHT")
  characterDefault(chr)
  chr:register()
end
