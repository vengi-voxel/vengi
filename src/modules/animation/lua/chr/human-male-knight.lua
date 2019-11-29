require 'chr.bones'
require 'chr.shared'

function init()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "head/knight")
  chr.setPath("belt", "belt/knight")
  chr.setPath("chest", "chest/knight")
  chr.setPath("pants", "pants/knight")
  chr.setPath("hand", "hand/knight")
  chr.setPath("foot", "foot/knight")
  chr.setPath("shoulder", "shoulder/knight")

  chr.setNeckHeight(0.6)
  chr.setHeadHeight(11.0)
end
