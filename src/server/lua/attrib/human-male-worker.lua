require "attrib.shared"

function registerHumanWorker()
  local chr = attrib.createContainer("HUMAN_MALE_WORKER")
  characterDefault(chr)
  chr:register()
end
