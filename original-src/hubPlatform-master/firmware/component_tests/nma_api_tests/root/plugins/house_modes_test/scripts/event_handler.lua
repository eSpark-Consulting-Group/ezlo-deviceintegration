print( "house_modes event handler" )

local params = ...
local core = require "core"

if params.event then
    core.send_ui_broadcast( params )
end