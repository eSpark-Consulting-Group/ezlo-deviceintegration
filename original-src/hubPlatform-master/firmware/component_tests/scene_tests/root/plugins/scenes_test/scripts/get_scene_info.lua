local scenes = require "scenes"
local core = require "core"

local params = ...
--
local id = params[ "id" ]
--
local info = scenes.get_scene_info(id)
core.send_ui_broadcast( info )