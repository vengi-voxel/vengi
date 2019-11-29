require 'chr.bones'
require 'chr.shared'

function init()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "head/blacksmith")
  chr.setPath("belt", "belt/blacksmith")
  chr.setPath("chest", "chest/blacksmith")
  chr.setPath("pants", "pants/blacksmith")
  chr.setPath("hand", "hand/blacksmith")
  chr.setPath("foot", "foot/blacksmith")
  chr.setPath("shoulder", "shoulder/blacksmith")
end
