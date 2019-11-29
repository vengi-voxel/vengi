require 'chr.bones'

function init()
  setupBones()
  chr.setRace("undead")
  chr.setGender("male")
  chr.setPath("head", "head/default")
  chr.setPath("belt", "belt/default")
  chr.setPath("chest", "chest/default")
  chr.setPath("pants", "pants/default")
  chr.setPath("hand", "hand/default")
  chr.setPath("foot", "foot/default")
  chr.setPath("shoulder", "")

  chr.setHeadHeight(14.0)
  chr.setChestHeight(1.0)
  chr.setInvisibleLegHeight(0.4)
  chr.setFootRight(-1.2)
end
