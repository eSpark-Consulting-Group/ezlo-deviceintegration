local scenes = require "scenes"
local core = require "core"

local params = ... or {}
--
local value = scenes.set_expression(params["name"], {
    code = params["code"],
    value_type = params["value_type"],
    metadata = params["metadata"],
    params = params["params"]
})
core.send_ui_broadcast(value)