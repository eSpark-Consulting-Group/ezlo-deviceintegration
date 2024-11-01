local scenes = require "scenes"
local core = require "core"

local params = ...
local name = params["name"]
local options = params["options"]

local value
if options == nil then
    value = scenes.get_expression_value(name)
else
    value = scenes.get_expression_value(name, options)
end

core.send_ui_broadcast(value)