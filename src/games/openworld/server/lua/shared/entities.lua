local module = {}

module.entities = {
  "animal-rabbit",
  "animal-wolf",
  "dwarf-male-blacksmith",
  "human-male-blacksmith",
  "human-male-knight",
  "human-male-shepherd",
  "human-male-worker",
  "human-female-worker",
  "undead-male-skeleton",
  "undead-male-zombie"
}

function module.register(prefix)
  for key, value in ipairs(module.entities) do
    local name = prefix .. '.' .. value
    local mod = require(name)
    mod.register()
  end
end

return module
