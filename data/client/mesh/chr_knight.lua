function init()
  mesh.setScale(vec3.new(0.4))
  -- mesh.addSkin('chr_knight.png')
  mesh.addAnimation('walk',  93.8, 125.0,  22)
  mesh.addAnimation('run',   93.8, 125.0,  22)
  mesh.addAnimation('jump',  25.0,  81.0,  22)
end
