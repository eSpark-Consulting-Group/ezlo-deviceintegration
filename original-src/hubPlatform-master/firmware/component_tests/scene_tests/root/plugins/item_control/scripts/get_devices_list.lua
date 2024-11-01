local params = ...
local core = require "core"

function dump( arg, prefix )
    prefix = prefix or 'empty'
    -- print("Dump called: " .. prefix)
    if type(arg) == 'table' then
        for k,v in pairs(arg) do
            -- print( (prefix or '') .. '.' .. k .. ', typeof: ' .. type(v) )
            dump(v, prefix .. '.' .. k)
        end
    else
        print(prefix .. ': ' .. tostring(arg))
    end
end

dump(params, 'params')

if params.testSelector == "get_devices" then
    print('Sending broadcasts...')

    local devices = core.get_devices()
    print('Devices fetched from Core: ' )
    dump( devices, 'devices' )
    core.send_ui_broadcast({ devices = devices })
elseif params.testSelector == "add_device" then
    -- No gatewayId from now on should be passed, or it will be ignored!
    local persistentDevice = {
        name = "some_persistent_device",
        type = "sensor",
        device_type_id = "security_sensor_type_id",
        category = "security_sensor",
        battery_powered = true,
        -- optional
        persistent = true
    }
    local err, device_id = pcall( core.add_device, persistentDevice )
    if not err then
        print("device created, id: " .. device_id)
    else
        print("Error calling core.add_device: " .. device_id)
    end
else
    print('Nothing to do')
end