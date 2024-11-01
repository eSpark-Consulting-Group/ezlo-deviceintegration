local params = ...
    if not params then
        print("Parameters are empty. Expected: data")
        return
    end

local security = require('security')
if not security then
    print('Failed to load security module')
    return
end

params.value = security.to_base64( params.data )

local core = require('core')
if not core then
    print('Failed to load core module')
    return
end
core.send_ui_broadcast( params )
