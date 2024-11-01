local params = ...
local http_server = assert(require('http_server'), 'Could not create http_server module')
assert(type(params) == 'table', 'Invalid argument type, table required')

local status, value = pcall(http_server.get_link_info, params.link_id)
if not status then
    print('ERROR: http_server.get_link_info failed, value=' .. value)
    params.error = value
else
    params.value = value
end

require('core').send_ui_broadcast(params)
