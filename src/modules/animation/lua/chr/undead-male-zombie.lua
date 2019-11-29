require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("undead", "male")
  chr.setPath("head", "default")
  chr.setPath("belt", "default")
  chr.setPath("chest", "default")
  chr.setPath("pants", "default")
  chr.setPath("hand", "default")
  chr.setPath("foot", "default")
  chr.setPath("shoulder", "")

  chr.setHeadHeight(14.0)
  chr.setChestHeight(1.0)
  chr.setInvisibleLegHeight(0.4)
  chr.setFootRight(-1.2)
end
