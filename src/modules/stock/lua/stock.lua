function init()
  stock.createItem(0, 'WEAPON', 'axe-1'):setSize(1, 1)
  stock.createItem(1, 'WEAPON', 'hammer-1'):setSize(1, 1)
  stock.createItem(2, 'WEAPON', 'sword-1'):setSize(1, 1)

  local invMain = stock.createContainer(1, "main")
  local invMainShape = invMain:shape()
  invMainShape:addRect(0, 0, 1, 1)

  local invRightHand = stock.createContainer(2, "weapon")
  local invRightHandShape = invRightHand:shape()
  invRightHandShape:addRect(0, 0, 1, 1)
end
