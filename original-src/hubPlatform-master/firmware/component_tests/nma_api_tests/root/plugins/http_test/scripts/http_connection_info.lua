local params = ...
if not params or not params.id then
    local msg = "Parameters are empty. Expected: id"
    print(msg)
    return
end

local http = require('http')
if not http then
    print('Failed to load http module')
    return
end

local id = params.id

local core = require('core')
print('http.get_connection_info: -> ')
local status, value = pcall(http.get_connection_info, id)

if not status then
    print('Error in get_connection_info(' .. id .. '): ' .. value);
    params.error = value
else
    params.connection_info = value
end

core.send_ui_broadcast(params)