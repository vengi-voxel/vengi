require 'chr.bones'

function init()
  setupBones()
  chr.setRace("human")
  chr.setGender("male")
  chr.setPath("head", "head/worker")
  chr.setPath("belt", "belt/worker")
  chr.setPath("chest", "chest/worker")
  chr.setPath("pants", "pants/worker")
  chr.setPath("hand", "hand/worker")
  chr.setPath("foot", "foot/worker")
  chr.setPath("shoulder", "")
end
