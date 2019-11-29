require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("undead", "male")
  setAllPaths("zombie")
  settings.setPath("shoulder", "")

  local attributes = defaultSkeletonAttributes()
  attributes["headHeight"] = 14.0
  attributes["chestHeight"] = 1.0
  attributes["invisibleLegHeight"] = 0.4
  attributes["footRight"] = -1.2
  return attributes
end
