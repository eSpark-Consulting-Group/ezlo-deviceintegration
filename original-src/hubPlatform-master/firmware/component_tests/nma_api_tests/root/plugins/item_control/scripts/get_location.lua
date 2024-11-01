local core = require "core"

local location = core.get_location()
core.send_ui_broadcast( location )