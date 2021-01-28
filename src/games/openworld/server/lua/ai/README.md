# Behaviour Trees

The behaviour trees are written in lua.

See the documentation given in the [ai module](../../../modules/ai/README.md)

# Functions

## `AI`

`createTree(name)`: Create a new behaviour tree.

## `Tree`

The object returned by `createTree` calls. The following methods are included:
* `createRoot(id, name)`: The `id` if the node factory id as given while registering nodes. The name should match the npc type as given in the [network protocol definition](../../../modules/network/definitions/Shared.fbs).
* `getName()`: Return the name of the root node

## `Node`

The object returned by `createRoot`. The following methods are included:
* `addNode(id, name)`: The `id` if the node factory id as given while registering nodes. The name can be freely chosen.
* `getName`: Return the name of the current node
* `setCondition(conditionExpression)`: See the simpleai documentation for more details about the syntax. It might also be a good idea to check the existing behaviour trees or the unit tests.

# Nodes

The behaviour tree nodes can either be implemented in C++ - see [src/modules/backend/entity/ai/action](../../../modules/backend/entity/ai/action) for examples or in lua (see  [src/server/lua/behaviourtreenodes.lua](../behaviourtreenodes.lua) for examples.

## LUARegistry

The lua registry exposes functions to write lua based ai tree nodes (actions, conditions, filter, ...).

This is most likely not complete and if you write a node in lua you will probably find objects or function not yet exposed that would be needed for your script to work. In that case, just go ahead and extend the [LUARegistry.cpp](../../../modules/ai/LUARegistry.cpp).
