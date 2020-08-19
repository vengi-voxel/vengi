local bones = require 'animal.bird-bones'
local birdshared = require 'animal.bird-shared'
local shared = require 'animal.shared'

function init()
  settings.setMeshTypes("head", "foot", "wing", "body")
  settings.setType("bird")
  bones.setupBones()
  shared.setBasePath("chicken")

  return birdshared.defaultSkeletonAttributes()
end
