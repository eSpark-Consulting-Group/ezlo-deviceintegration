local params = ...
if not params then
    print(
        "Parameters are empty. Expected: command: [subscribe | unsubscribe], data: { path: string, operations: undefined | string[]} | sessionId: number"
    )
    return
end

local core = require("core")
local _cloud_storage = require("cloud_storage")

if (params.command == "fetch_token") then
    local token = _cloud_storage.fetch_token()
    print("cloud_storage.fetch_token: -> ")
    params.token_data = token
elseif (params.command == "fetch_endpoints") then
    local table = params.table
    local result = _cloud_storage.fetch_endpoints(table)
    print("cloud_storage.fetch_endpoints: -> ")
    params.endpoints_data = result
elseif (params.command == "fetch_endpoint") then
    local name = params.name
    local options = params.options
    local result = _cloud_storage.fetch_endpoint(name,options)
    print("cloud_storage.fetch_endpoint: -> ")
    params.url = result
elseif (params.command == "subscribe") then
    local result = _cloud_storage.subscribe( "HUB:cloud_storage/scripts/event_handler" )
    params.id = result
    print("cloud_storage.subscribe: -> ")
end

core.send_ui_broadcast(params)
