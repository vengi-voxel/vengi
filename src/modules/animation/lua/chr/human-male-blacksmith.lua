require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "blacksmith")
  chr.setPath("belt", "blacksmith")
  chr.setPath("chest", "blacksmith")
  chr.setPath("pants", "blacksmith")
  chr.setPath("hand", "blacksmith")
  chr.setPath("foot", "blacksmith")
  chr.setPath("shoulder", "blacksmith")
end
