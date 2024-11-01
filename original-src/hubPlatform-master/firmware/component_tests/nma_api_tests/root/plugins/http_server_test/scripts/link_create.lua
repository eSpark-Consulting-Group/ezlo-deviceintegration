local params = ...
local http_server = assert(require('http_server'), 'Could not create http_server module')
assert(type(params) == 'table', 'Invalid argument type, table required')

local status, value = pcall(http_server.create_link_url, params)
if not status then
    print('ERROR: http_server.create_link_url failed')
    params.error = value
else
    params.value = value
end

require('core').send_ui_broadcast(params)
