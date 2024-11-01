local params = ...
local core = require "core"
local device = core.get_device(params.id)
core.send_ui_broadcast({ device = device })
