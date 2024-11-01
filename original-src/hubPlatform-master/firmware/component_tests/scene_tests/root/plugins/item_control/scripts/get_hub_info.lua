local core = require "core"

local info = core.get_hub_info()
core.send_ui_broadcast( info )
