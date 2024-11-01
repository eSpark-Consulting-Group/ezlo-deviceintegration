local params = ...

local command = params.command

local core = assert(require("core"), "Fail to load core functionality")
local zwave = assert(require "zwave", "Fail to load zwave functionality")

if params.command == "get_provisioning_info" then
    local status, err_or_value = pcall(zwave.get_provisioning_info, params.node_id)
    if not status then
        core.send_ui_broadcast({ data = { error = err_or_value } })
    else
        core.send_ui_broadcast({ data = err_or_value })
    end
end

if params.command == "set_provisioning_info" then
    local status, err_or_value = pcall(zwave.set_provisioning_info, params.node_id, params.name, params.room)
    if not status then
        core.send_ui_broadcast({ data = { error = err_or_value } })
    end
end
