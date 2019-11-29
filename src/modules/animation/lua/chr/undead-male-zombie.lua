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

  local attributes = defaultSkeletonAttributes()
  attributes["headHeight"] = 14.0
  attributes["chestHeight"] = 1.0
  attributes["invisibleLegHeight"] = 0.4
  attributes["footRight"] = -1.2
  return attributes
end
