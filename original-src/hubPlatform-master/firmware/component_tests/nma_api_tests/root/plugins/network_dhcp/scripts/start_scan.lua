local params = ...
if not params then
    print("Parameters are empty. Expected: command: [start_scan], data: { path: string, ip_addresses: string}")
    return
end

local network = require('network')
if not network then
    print('Failed to load network module')
    return
end

if params.command ~= nil then
    if params.command == 'start_scan' then
        local scan_id = network.start_scan_network(params.script, params.ip_addresses, 'IPv4')
        print('start_scan.lua: network.start_scan_network: -> ' .. scan_id)

        params.value = scan_id
        local core = require('core')
        core.send_ui_broadcast(params)
    elseif params.command == 'stop_scan' then
        network.stop_scan_network(params.scan_id)
        print('start_scan.lua: network.stop_scan_network: -> ' .. scan_id)

        params.value = scan_id
        local core = require('core')
        core.send_ui_broadcast(params)
    end
else
    print('start_scan.lua: Some data received')
    if params.event ~= nil then
        print('start_scan.lua: Event data received: ', params.event)
    else
        print('start_scan.lua: Some data received')
    end
end
