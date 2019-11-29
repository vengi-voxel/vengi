require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("human", "male")
  settings.setPath("head", "knight")
  settings.setPath("belt", "knight")
  settings.setPath("chest", "knight")
  settings.setPath("pants", "knight")
  settings.setPath("hand", "knight")
  settings.setPath("foot", "knight")
  settings.setPath("shoulder", "knight")

  skeleton.setNeckHeight(0.6)
  skeleton.setHeadHeight(11.0)
end
