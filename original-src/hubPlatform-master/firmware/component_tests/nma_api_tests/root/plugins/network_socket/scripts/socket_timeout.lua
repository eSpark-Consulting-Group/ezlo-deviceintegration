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

local handle =network.udp()

local core = require('core')

local value = 'unknown result'

if (params.command == 'set_timeout') then 
    value = network.set_timeout(handle, params.timeout)
    print('network.set_timeout: -> ')
    print(value)
elseif (params.command == 'setoption') then 
    value = network.setoption(handle, params.option, params.jsonData)
    print('network.setoption: -> ')
    print(value)
end

params.value = value
core.send_ui_broadcast( params )