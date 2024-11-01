local scenes = require "scenes"
local core = require "core"
local params = ... or {}
local scene_id = params[ "scene_id" ]
local block_id = params[ "block_id" ]
local info = scenes.get_scene_when_block_state(scene_id, block_id)
core.send_ui_broadcast( info )
