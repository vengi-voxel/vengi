require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "knight")
  chr.setPath("belt", "knight")
  chr.setPath("chest", "knight")
  chr.setPath("pants", "knight")
  chr.setPath("hand", "knight")
  chr.setPath("foot", "knight")
  chr.setPath("shoulder", "knight")

  chr.setNeckHeight(0.6)
  chr.setHeadHeight(11.0)
end
