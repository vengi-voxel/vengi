require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("undead", "male")
  chr.setPath("head", "skeleton")
  chr.setPath("belt", "skeleton")
  chr.setPath("chest", "skeleton")
  chr.setPath("pants", "skeleton")
  chr.setPath("hand", "skeleton")
  chr.setPath("foot", "skeleton")
  chr.setPath("shoulder", "skeleton")

  chr.setHandRight(-6.5)
  chr.setShoulderRight(-3.0)
  chr.setShoulderForward(-1.0)
  chr.setChestHeight(5.0)
  chr.setBeltHeight(1.0)
  chr.setInvisibleLegHeight(0.4)
  chr.setFootRight(-3.2)
end
