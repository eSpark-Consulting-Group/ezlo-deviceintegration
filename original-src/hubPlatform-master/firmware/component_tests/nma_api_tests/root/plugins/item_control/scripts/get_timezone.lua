local core = require "core"

local timezone = core.get_timezone()
core.send_ui_broadcast( { ["timezone" ] = timezone } )