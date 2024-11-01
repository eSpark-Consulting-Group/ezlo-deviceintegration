local scenes = require "scenes"
local core = require "core"

local params = ... or {}
--
local value = scenes.set_variable(params["name"], {
    value = params["value"],
    value_type = params["value_type"],
    metadata = params["metadata"]
})
core.send_ui_broadcast(value)