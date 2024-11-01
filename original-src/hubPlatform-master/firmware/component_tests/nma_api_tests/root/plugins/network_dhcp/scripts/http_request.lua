-- part of http_test test suite (HttpLuadTests)
local params = ...
if not params then
    print("Parameters are empty. Expected: command: request, http.request params")
    return
end

local http = require('http')
if not http then
    print('Failed to load http module')
    return
end

local core = require('core')

local value = 'unknown result'

if (params.command == 'request') then
    local status, err_or_value = pcall(http.request, params);
    if not status then
        local error = { error = err_or_value }
        core.send_ui_broadcast(error)
    else
        print('http.request: -> ')
        value = err_or_value
        print(value);
    end
end

params.value = value
core.send_ui_broadcast(params)