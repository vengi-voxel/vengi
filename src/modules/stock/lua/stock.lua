function init()
  stock.createItem(0, 'WEAPON', 'axe-1'):setSize(1, 1):addLabel('anim', 'stroke')
  stock.createItem(1, 'WEAPON', 'hammer-1'):setSize(1, 1):addLabel('anim', 'stroke')
  stock.createItem(2, 'WEAPON', 'sword-1'):setSize(1, 1):addLabel('anim', 'swing')
  stock.createItem(3, 'WEAPON', 'sword-2'):setSize(1, 1):addLabel('anim', 'swing')

  local invMain = stock.createContainer(1, "main")
  local invMainShape = invMain:shape()
  invMainShape:addRect(0, 0, 1, 1)

  local invRightHand = stock.createContainer(2, "weapon")
  local invRightHandShape = invRightHand:shape()
  invRightHandShape:addRect(0, 0, 1, 1)
end
