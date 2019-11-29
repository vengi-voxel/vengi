require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("undead", "male")
  settings.setPath("head", "default")
  settings.setPath("belt", "default")
  settings.setPath("chest", "default")
  settings.setPath("pants", "default")
  settings.setPath("hand", "default")
  settings.setPath("foot", "default")
  settings.setPath("shoulder", "")

  skeleton.setHeadHeight(14.0)
  skeleton.setChestHeight(1.0)
  skeleton.setInvisibleLegHeight(0.4)
  skeleton.setFootRight(-1.2)
end
