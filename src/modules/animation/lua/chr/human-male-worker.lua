require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "worker")
  chr.setPath("belt", "worker")
  chr.setPath("chest", "worker")
  chr.setPath("pants", "worker")
  chr.setPath("hand", "worker")
  chr.setPath("foot", "worker")
  chr.setPath("shoulder", "")
end
