require "attrib.shared"

function registerWorker()
  local chr = attrib.createContainer("HUMAN_MALE_WORKER")
  characterDefault(chr)
  chr:register()
end
