## Access control
* Record number of violations and kick player if too many in a rolling time interval
* Restrict chat channel access -- store chat channels & ACLs
* Better login/register/update UI
* Roles -> overlay set of privs
* Privs for offline players
* Area protection system
  * Explicit deny for dig/place
* Hard limits on node/mapblock access (+/- 2^31-1) -- throw error

## Performance
* renderer.js hotspots
* GC spikes
* UI generation speed
* Debug when set_node is being combined - ARM vs x86
* Purge old IDtoIS entries on save?
* Faster client lighting (similar to server)
* Test unordered_map
* set_mapblock*s*?
* Global itemstring compression table?
* Throttle (with burst?) for player actions like setting nodes and loading mapblocks and causing the server to prep mapblocks
* Built in stats.js
* Batch mapblock request/response? -- use array instead of object
* SIMD?
* DB access profiling
* Faster lighting
* Time mapblock generation w/ DEBUG_PERF

## Commands / chat
* Teleport to player/player to you & teleport requests
* View chat history
* Arrow up/down w/chat -- command/chat history
* Admin CLI on stdin
* PM command
* Nice auto-generated names with words for guest players
* Chat channels & channel ACLs
* Chat & player name & channel anti-abuse

## UI
* UI for keybindings & other config
* UI for help?
* Render distance options in GUI
  * Render frustrum culling
* Fix login form submit in Firefox
* Login SSO
* Creative inventory search
* Differential ui_update
* Nested UI definition -- much more flexibility
* Loading screen?
* Better menu screen
* Inventory -- shift-right-click/scroll/shift-scroll/etc.
* Scrollbars
* Overlay when disconnected

## Visuals
* Skybox -- change colors etc. (incl. night) -- proper skybox
* Different color temperature for sunlight and artificial light?
* Pointy box should match node collision/mesh box
* Third-person view option
* Animated textures (torch, water)
* No backface culling on glass
* Fully connecting glass
* Fence lighting looks unnatural
* Textures - composite textures similar to MT
* Textures - fix registration/use race condition
* Bleed in light from edges after recompute?

## Visuals (4th dimension)
* Fast peek option (only render 1 camera)
* Firefox peek compositing performance
* Peek for players

## Glitches
* Fix Firefox glitches
* Fix sun updates
  * Prioritize lighting updates from server
  * Lighting updates not always shown by originating client /
    smooth lighting seams / etc. / at y=0
  * Client not always catching light updates
* Fix Firefox highlight bug
* Firefox inv highlight
* Don't fall when client out of focus -- request OOB mapblock

## Server map stuff
* Fast path lighting -- pull mapblocks and spread sunlight deeper
* Make water destroy some nodes (like plants)
* ABM system: grass and flowers spreading, for example
* Random tick system (grass growth/spread, farming, etc.)
* Leaf degeneration
* Break flowers/etc. when node below broken

## Mapgen
* Survival-ready mapgen
* Ores
* Caves
* Mapgen biomes + fancier noise
* Clay underwater

## Player
* Handling for build-in-player
* Sneak
* Limit player fall speed
* Player model & floating name
* Food/stamina
* Head bobbing while walking
* Better sprint key
* Noclip
* Restart digging when wield changed

## Server improvements
* validate_config -- issue warnings or errors for bad config
* Declarative command input validation
  * regex? required privs? different command variants?
* Throw/catch for client request errors (server_message.cpp)
* Use const/const ref where appropriate
* More logging info -- event type
* Lock cerr in log
* Ticks delayed when server busy -- fix
  * Want to be able to do basic movement validation

## DB
* Fixed-endian DB storage
* DB - use text compression on IDtoIS?
* DB - clear old IDtoIS entries on save

## Client improvements
* Rework client window system

## Items (easy)
* More doors (doors mod)
* Dye mixing/flower sourcing
* Marble!
* Light flowers?
* Quartz + variants
  * Pillars
* Metal pillar beam
* Amethyst
* Sponge?
* Basalt
* Obsidian glass
* Asphalt
* Metal ladder
* Ice, ice bricks

## Items (new features)
* Wall torches
* Make torches look nicer
* Redstone
* Signs
* Magma blocks
* Minecarts
* TNT
* Liquids
  * Lava
  * Acid
  * Interdimensional slime
* Farming

## Items (other)
* Comprehensive cook recipies
* Comprehensive craft recipies
* Fix oddly_breakable_by_hand (torches)
* Unknown item shouldn't be stackable
* Set ladder sunkLit?
* Stairs lighting issue

## Space
* 4th dimension
  * Portals
  * Optional see through portals
* Proximity chat

## Speculation
* Tests or something
* 4th dimension
  * Inception dreams
* Gravity mechanics (space)
* Light based on vertex normals -- artificial light on one axis, sunlight on another?
* Atmosphere/space skybox?

## Map tools
* Worldedit
* Screwdriver and node replacer
* More flexible slab rotation
* Different restrict rot on place for chest/furnace/door

## Lighting
* More efficient & correct sunlight calculations: check node above (if in up-to-date mapblock) for sunlight_level=15

