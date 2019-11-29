require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("undead", "male")
  settings.setPath("head", "skeleton")
  settings.setPath("belt", "skeleton")
  settings.setPath("chest", "skeleton")
  settings.setPath("pants", "skeleton")
  settings.setPath("hand", "skeleton")
  settings.setPath("foot", "skeleton")
  settings.setPath("shoulder", "skeleton")

  skeleton.setHandRight(-6.5)
  skeleton.setShoulderRight(-3.0)
  skeleton.setShoulderForward(-1.0)
  skeleton.setChestHeight(5.0)
  skeleton.setBeltHeight(1.0)
  skeleton.setInvisibleLegHeight(0.4)
  skeleton.setFootRight(-3.2)
end
