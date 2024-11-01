local params = ... or {}

local log = require("logging")

local user_data = params["user_data"]
local timer = require("timer")
local core = require("core")

if params["command"] ~= nil then
    if params["command"] == "cancel_timer" then
        timer.cancel(params['timer_id'])
        core.send_ui_broadcast({value = {destroyed = params['timer_id']}})
    elseif params["command"] == "test_set_timeout" then
        local timer_id = timer.set_timeout(1000, "HUB:network_dhcp/scripts/timer_tests", user_data)
        log.info("Created timer_id : " .. timer_id)
        core.send_ui_broadcast({value = {command = params["command"], timer_id = timer_id}})
    elseif params["command"] == "test_set_timeout_with_id" then
        timer.set_timeout_with_id(1000, "123", "HUB:network_dhcp/scripts/timer_tests", user_data)
        log.info("Created timer_with_id : 123")
        core.send_ui_broadcast({value = {command = params["command"], timer_id = 123}})
    elseif params["command"] == "test_set_interval" then
        local timer_interval = timer.set_interval(1000, "HUB:network_dhcp/scripts/timer_tests", user_data)
        log.info("Created timer_interval : " .. timer_interval)
        core.send_ui_broadcast({value = {command = params["command"], timer_id = timer_interval}})
    elseif params["command"] == "test_set_interval_with_id" then
        timer.set_timeout_with_id(1000, "456", "HUB:network_dhcp/scripts/timer_tests", user_data)
        log.info("Created timer_interval_with_id : 456")
        core.send_ui_broadcast({value = {command = params["command"], timer_id = 456}})
    else
        log.error("Error: unsupported command passed: " .. params["command"])
        core.send_ui_broadcast({value = {command = params["command"]}})
    end
else
    log.info("Handle timer event here: ", params)
    core.send_ui_broadcast({value = {command = params["command"]}})
end