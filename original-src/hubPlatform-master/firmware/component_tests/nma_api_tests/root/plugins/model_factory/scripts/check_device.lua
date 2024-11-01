print( "Check Device" )

local params = ...
if not params then
    print("Parameters are empty")
    return
end

if not params.device_id then
    print("device_id is empty")
    return
end

if not params.check then
    print("Checking both")
end

local core = require "core"
if not params.items then
    core.send_device_checked_broadcast( {
        device_id = params.device_id,
        status = "finished",
        completed = 100
    } )
else
    core.send_device_checked_broadcast( {
        device_id = params.device_id,
        status = "failed",
        error = {
            code = tonumber(params.items[1]),
            data = params.items[2],
            message = params.items[3]
        }
    } )
end

