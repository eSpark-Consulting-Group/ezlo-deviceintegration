print( "Core event handler" )

local params = ...
local core = require "core"

core.send_ui_broadcast( params )


