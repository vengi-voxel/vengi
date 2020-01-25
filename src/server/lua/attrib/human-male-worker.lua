require "attrib.shared"

function registerHumanMaleWorker()
  local chr = attrib.createContainer("HUMAN_MALE_WORKER")
  characterDefault(chr)
  chr:register()
end
