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

  local attributes = defaultSkeletonAttributes()
  attributes["neckHeight"] = 0.6
  attributes["headHeight"] = 11.0
  return attributes
end
