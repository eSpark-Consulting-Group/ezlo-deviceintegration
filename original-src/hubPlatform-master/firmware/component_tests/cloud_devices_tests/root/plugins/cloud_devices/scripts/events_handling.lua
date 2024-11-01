local params = ...

local maybe_command = ""

if params.id then
    maybe_command = params.id
elseif params.command_id then
    maybe_command = params.command_id
end

-- Commands:
--   test.event_params
--   test.get_all_devices_ids
--   test.get_device(<device_id>)
--   test.send_command(<command_id>,<device_id>,<capability>,<command>,<params>)

if maybe_command:match("^test[.]") then
    local core = assert(require("core"), "Fail to load the core functionality")
    local cloud_devices = assert(require "cloud_devices", "Fail to load cloud devices functionality")

    if maybe_command:match("^test[.]event_params") then
        core.send_ui_broadcast({ data = params })
    end

    if maybe_command:match("^test[.]get_all_devices_ids") then
        local devices_ids = cloud_devices.get_all_devices_ids()
        core.send_ui_broadcast({ data = devices_ids })
    end

    if maybe_command:match("^test[.]get_device") then
        local device_id = maybe_command:match("[(]([^)]+)[)]")
        local device = cloud_devices.get_device(device_id)
        core.send_ui_broadcast({ data = device })
    end

    if maybe_command:match("^test[.]send_command") then
        if params.event ~= "device_removed" then
            local command_id, device_id, capability, command, rawParams = maybe_command:match("[(]'([^']+)', '([^']+)', '([^']+)', '([^']+)', '([^']+)'[)]")
            local parsedParams = load("return " .. rawParams)()
            local status, error_msg = pcall(cloud_devices.send_command, command_id, device_id, capability, command, parsedParams)
            if not status then
                core.send_ui_broadcast({ data = { error = error_msg } })
            end
        end
    end
end
