require 'animal.bird-bones'
require 'animal.bird-shared'

function init()
  settings.setMeshTypes("head", "foot", "wing", "body")
  settings.setType("bird")
  setupBones()
  setBasePath("chicken")

  return defaultSkeletonAttributes()
end
