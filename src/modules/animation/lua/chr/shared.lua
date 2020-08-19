local module = {}

function module.setBasePath(race, gender)
  settings.setBasePath("models/characters/" .. race .. "/" .. gender)
end

function module.setupMeshTypes()
  settings.setType("character")
  settings.setMeshTypes("head", "chest", "belt", "pants", "hand", "foot", "shoulder", "glider")
end

function module.setAllPaths(path)
  local meshes = { "head", "chest", "belt", "pants", "hand", "foot", "shoulder" }
  for i, name in ipairs(meshes) do
    settings.setPath(name, path)
  end
end

function module.defaultSkeletonAttributes()
  local attributes = {
    scaler = 1.0,
    toolRight = 6.0,
    toolForward = -6.1,
    toolScale = 1.0,
    neckRight = 0.0,
    neckForward = 0.0,
    neckHeight = 0.0,
    headScale = 1.0,
    handRight = -7.5,
    handForward = 0.0,
    shoulderRight = -5.0,
    shoulderForward = 0.0,
    runTimeFactor = 12.0,
    jumpTimeFactor = 14.0,
    idleTimeFactor = 0.3,
    shoulderScale = 1.1,
    -- to shift the rotation point for the feet
    hipOffset = 6.0,
    origin = 0.0,
    footHeight = 3.0,
    invisibleLegHeight = 0.5,
    pantsHeight = 3.0,
    beltHeight = 2.0,
    chestHeight = 5.0,
    headHeight = 9.0,
    gliderOffset = 5.0,
    glidingForward = 2.0,
    glidingUpwards = 2.0,
    footRight = -3.2
  }
  return attributes
end

return module
