local params = ... or {}
if not params then
    print( 'Parameters are empty.' )
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

local status, valueOrError = pcall( house_modes.add_protect_sos_button, params.device_id, params.service )
if status then
    params.finished = 1
else
    params.finished = 0
    print( 'Error:' .. tostring( valueOrError ) )
end
core.send_ui_broadcast( params )
