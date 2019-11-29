require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "shepherd")
  chr.setPath("belt", "shepherd")
  chr.setPath("chest", "shepherd")
  chr.setPath("pants", "shepherd")
  chr.setPath("hand", "shepherd")
  chr.setPath("foot", "shepherd")
  chr.setPath("shoulder", "shepherd")
end
