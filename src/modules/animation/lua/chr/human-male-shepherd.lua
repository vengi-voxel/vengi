require 'chr.bones'
require 'chr.shared'

function init()
  setupBones()
  setBasePath("human", "male")
  chr.setPath("head", "head/shepherd")
  chr.setPath("belt", "belt/shepherd")
  chr.setPath("chest", "chest/shepherd")
  chr.setPath("pants", "pants/shepherd")
  chr.setPath("hand", "hand/shepherd")
  chr.setPath("foot", "foot/shepherd")
  chr.setPath("shoulder", "shoulder/shepherd")
end
