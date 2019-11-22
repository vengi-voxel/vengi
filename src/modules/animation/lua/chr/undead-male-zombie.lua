require 'chr.bones'

function init()
  setupBones()
  chr.setRace("undead")
  chr.setGender("male")
  chr.setHead("head/default")
  chr.setBelt("belt/default")
  chr.setChest("chest/default")
  chr.setPants("pants/default")
  chr.setHand("hand/default")
  chr.setFoot("foot/default")
  chr.setShoulder("")

  chr.setHeadHeight(14.0)
  chr.setChestHeight(1.0)
  chr.setInvisibleLegHeight(0.4)
  chr.setFootRight(-1.2)
end
