require 'chr.bones'
require 'chr.shared'

function init()
  setupMeshTypes()
  setupBones()
  setBasePath("undead", "male")
  setAllPaths("skeleton")

  local attributes = defaultSkeletonAttributes()
  attributes["handRight"] = -6.5
  attributes["shoulderRight"] = -3.0
  attributes["shoulderForward"] = -1.0
  attributes["chestHeight"] = 5.0
  attributes["beltHeight"] = 1.0
  attributes["invisibleLegHeight"] = 0.4
  attributes["footRight"] = -3.2
  return attributes
end
