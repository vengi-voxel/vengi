require 'chr.bones'

function init()
  setupBones()
  chr.setRace("human")
  chr.setGender("male")
  chr.setHead("head/knight")
  chr.setBelt("belt/knight")
  chr.setChest("chest/knight")
  chr.setPants("pants/knight")
  chr.setHand("hand/knight")
  chr.setFoot("foot/knight")
  chr.setShoulder("shoulder/knight")

  chr.setNeckHeight(0.6)
  chr.setHeadHeight(11.0)
end
