local module = {}

-- We set the filename the same as the type for animations. As we don't mix
-- them, we ensure that every entity is is an own directory
function module.setAllPaths()
  local meshes = settings.getMeshTypes()
  for i, name in ipairs(meshes) do
    settings.setPath(name, name)
  end
end

function module.setBasePath(type)
  settings.setBasePath("models/animals/" .. type)
  module.setAllPaths()
end

return module
