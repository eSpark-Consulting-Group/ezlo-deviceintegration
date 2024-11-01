local params = ...
    if not params then
        print("Parameters are empty. Expected: device")
        return
    end

local core = require('core')
if not core then
    print('Failed to load core module')
    return
end

core.modify_device( params.device_id, params.device )
