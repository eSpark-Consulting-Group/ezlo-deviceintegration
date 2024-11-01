local params = ...
if not params then
    print("Parameters are empty. Expected http event data.")
    return
end

local http = require('http')
local core = require('core')
local security = require("security")

local eventData = params.data

if params.event == "http_data_received" then
    print('http_binary_send: requestId: ' .. eventData.id .. ' event name: ' .. params.event .. ' -> ')
    local hash = security.md5(eventData.data)
    local to_send = { event = params.event, data_hash = hash, data = { code = eventData.code, id = eventData.id }, data_size = string.len(eventData.data) }
    core.send_ui_broadcast(to_send)
    print('http_binary_send: requestId: ' .. eventData.id .. ' broadcasted: ' .. to_send .. ' -> ')
elseif params.event == "http_connection_closed" then
    print('http_binary_send: requestId: ' .. eventData.id .. ", event: " .. params.event)
    core.send_ui_broadcast({ event = params.event, data = params.data })
end
