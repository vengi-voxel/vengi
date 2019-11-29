require 'chr.bones'

function init()
  setupBones()
  chr.setRace("undead")
  chr.setGender("male")
  chr.setPath("head", "head/skeleton")
  chr.setPath("belt", "belt/skeleton")
  chr.setPath("chest", "chest/skeleton")
  chr.setPath("pants", "pants/skeleton")
  chr.setPath("hand", "hand/skeleton")
  chr.setPath("foot", "foot/skeleton")
  chr.setPath("shoulder", "shoulder/skeleton")

  chr.setHandRight(-6.5)
  chr.setShoulderRight(-3.0)
  chr.setShoulderForward(-1.0)
  chr.setChestHeight(5.0)
  chr.setBeltHeight(1.0)
  chr.setInvisibleLegHeight(0.4)
  chr.setFootRight(-3.2)
end
