require 'chr.bones'

function init()
  setupBones()
  chr.setRace("undead")
  chr.setGender("male")
  chr.setHead("head/skeleton")
  chr.setBelt("belt/skeleton")
  chr.setChest("chest/skeleton")
  chr.setPants("pants/skeleton")
  chr.setHand("hand/skeleton")
  chr.setFoot("foot/skeleton")
  chr.setShoulder("shoulder/skeleton")

  chr.setHandRight(-6.5)
  chr.setShoulderRight(-3.0)
  chr.setShoulderForward(-1.0)
  chr.setChestHeight(5.0)
  chr.setBeltHeight(1.0)
  chr.setInvisibleLegHeight(0.4)
  chr.setFootRight(-3.2)
end
