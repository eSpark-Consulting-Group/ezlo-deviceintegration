print( "Get local time" )

local params = ...
local core = require "core"

local local_time = core.get_local_time( params[ "format" ] )
print( local_time )
core.send_ui_broadcast( { [ "localTime" ] = local_time } )
