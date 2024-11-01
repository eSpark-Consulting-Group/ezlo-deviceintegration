local scenes = require "scenes"
local core = require "core"

local params = ...
--
local name = params[ "name" ]
local options = params[ "options" ]
--
local value = scenes.get_expression_value(name, options)
core.send_ui_broadcast( value )