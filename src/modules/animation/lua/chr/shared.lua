function setBasePath(race, gender)
  chr.setBasePath("models/characters/" .. race .. "/" .. gender)
end

function setupMeshTypes()
  chr.setMeshTypes("head", "chest", "belt", "pants", "hand", "foot", "shoulder", "glider")
end
