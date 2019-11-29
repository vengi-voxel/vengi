function setBasePath(race, gender)
  settings.setBasePath("models/characters/" .. race .. "/" .. gender)
end

function setupMeshTypes()
  settings.setMeshTypes("head", "chest", "belt", "pants", "hand", "foot", "shoulder", "glider")
end
