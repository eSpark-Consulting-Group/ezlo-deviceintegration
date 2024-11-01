local params = ...

local _logging = require "logging"

local function dump_event(event)
    _logging.debug("Event started: scan_id: " .. event.scan_id)
    if event.ip_address ~= nil then
        _logging.debug("Event started: ip_address: " .. event.ip_address)
    end
    if event.mac ~= nil then
        _logging.debug("Event started: mac: " .. event.mac)
    end
    if event.host_name ~= nil then
        _logging.debug("Event started: host_name: " .. event.host_name)
    end
    if event.ports ~= nil then
        _logging.debug("Event started: ports: " .. tostring(event.ports))
    end
    if event.tries ~= nil then
        _logging.debug('Event started: tries: ' .. tostring(event.tries))
    end
    if event.errors ~= nil then
        _logging.debug('Event started: errors: ' .. tostring(event.errors))
    end
end

local function handle_events(event)
    dump_event(event)
    if event.status == "started" then
        _logging.debug("started")
    elseif event.status == "finished" then
        _logging.debug("finished, stop scanning")
        local network = require "network"
        pcall( network.stop_scan_network, event.scan_id )
    elseif event.status == "progress" then
        _logging.debug("progress")
        require('core').send_ui_broadcast( event )
    end
end

if params ~= nil and params.event ~= nil and params.event == "network" then
    local data = params.data
    if data.status ~= nil then
        _logging.debug("data: " .. data.status)
        handle_events(data)
    end

    return 0
end

local network = require "network"

local ip_list = params.ip_list
local ip_version = params.ip_version
local tries = params.tries or 1
if ip_version == nil then
    ip_version = "IPv4"
end
local handle_results = "HUB:network_dhcp/scripts/scan"
local scan_id = network.start_scan_network(handle_results, ip_list, ip_version, {}, tries)

_logging.info("scanning is started, scan id: " .. tostring(scan_id))
