require 'animal.shared'

function defaultSkeletonAttributes()
  local attributes = {
    scaler = 1.0,
    headScale = 1.0,
    bodyScale = 1.0,
    origin = 0.0,
    footHeight = 3.0,
    footRight = -3.2,
    wingHeight = 3.0,
    wingRight = -5.2,
    wingOffset = 2.0,
    invisibleLegHeight = 0.5,
    bodyHeight = 3.0,
    headHeight = 9.0
  }
  return attributes
end
