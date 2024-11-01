local params = ...
if not params then
    print( 'Parameters are empty. Expected: mode_id' )
    return
end

local house_modes = require( 'house_modes' )
if not house_modes then
    print( 'Failed to load security module' )
    return
end

local core = require( 'core' )
if not core then
    print( 'Failed to load core module' )
    return
end

params.value = house_modes.get_mode( params.mode_id )
core.send_ui_broadcast( params )
