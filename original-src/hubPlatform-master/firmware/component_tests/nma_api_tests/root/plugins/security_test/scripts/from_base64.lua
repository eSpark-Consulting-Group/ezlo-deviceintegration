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

if params.method == 'md5' then
    local encoded = security.from_base64( params.data )
    params.value = security.md5( encoded )
else
    params.value = security.from_base64( params.data )
end

local core = require('core')
if not core then
    print('Failed to load core module')
    return
end
core.send_ui_broadcast( params )
