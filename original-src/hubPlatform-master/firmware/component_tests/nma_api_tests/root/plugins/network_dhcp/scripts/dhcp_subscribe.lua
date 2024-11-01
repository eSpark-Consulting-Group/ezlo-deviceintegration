local params = ...
if not params then
    print("Parameters are empty. Expected: command: [subscribe | unsubscribe], data: { path: string, operations: undefined | string[]} | sessionId: number")
    return
end

local network = require('network')
if not network then
    print('Failed to load network module')
    return
end

local core = require('core')

local value = 'unknown result'

if (params.command == 'subscribe') then
    value = network.start_listening_dhcp(params.path, params.operations)
    print('network.start_listening_dhcp: -> ')
    print(value);
elseif (params.command == 'unsubscribe') then
    value = network.stop_listening_dhcp(params.id)
    print('network.stop_listening_dhcp: -> ')
    print(value)
end

params.value = value
core.send_ui_broadcast( params )