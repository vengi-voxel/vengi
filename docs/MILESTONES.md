Project Voxel
-------------------

# Milestone #1
* VoxelVolume on Server [done]
* Network message to connect and send the server seed to create the client side world [done]
* VoxelVolume on Client [done]
* Render VoxelVolume on the client side [done]
* Camera movement without clipping [done]

# Milestone #2
* Network handlers for user/irrlicht movement [done]
* Irrlich movement (clipping on the Voxels) and send movement messages to the server and back to the client [done]
** lerping between the messages
* Server needs some user representation [done]

# Milestone #3
* NPCs [done]
* Network messages for spawning npcs [done]
* Network messages for npc movement [done]
* Network handlers for spawning npcs [done]
* Network handlers for npc movement [done]
* Take over NPCs

# Milestone #4
* Behaviour trees for a simple NPC chain (Shephard => Trader)
* Visibility (QuadTree in the EntityStorage::update method) [done]

# Milestone #5
* Login (Authentification and Authorization - see EntityStorage::login)
* Logout -> NPC falls asleep for some time, NPC will follow BehaviourTree again after some time

# Milestone #6
* Bigger worlds
* Some sort of AMS [done]
* Persist user data
* Persist world data on server shutdown


Attribute handling
------------------

* Buffs/Debuffs
* AreaEffects
* Spells/Dispells
* Skills
* Cooldowns [done]
