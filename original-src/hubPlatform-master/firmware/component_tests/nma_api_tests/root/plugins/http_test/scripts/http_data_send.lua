local params = ...
if not params then
    print("Parameters are empty. Expected http event data.")
    return
end

local http = require('http')
local core = require('core')

local data = "Data to send from lua data_send event handler"
local eventData = params.data

if params.event == "http_data_send" then
    local nextStep = { id = eventData.id, data = data, last = true }
    print('http_data_send: requestId: ' .. eventData.id .. ', sendData() call -> ')
    print(nextStep)

    nextStep.source = "http_data_send event handler"
    core.send_ui_broadcast({ nextStep = nextStep, event = params.event })

    http.send_data(nextStep)
elseif params.event == "http_data_received" then
    print('http_data_send: requestId: ' .. eventData.id .. ' event name: ' .. params.event .. ' -> ')
    print("# eventData >>> id: " .. eventData.id)
    if string.len(eventData.data) > 2048 then
        pcall(print, "# eventData >>> data: size: "
                .. tostring(string.len(eventData.data))
                .. " >>>\n"
                .. string.sub(tostring(eventData.data), 1, 200)
                .. "\n...<<< (truncated) finished")
    else
        pcall(print, "# eventData >>> data: >>>\n" .. tostring(eventData.data) .. "\n<<< finished")
    end

    core.send_ui_broadcast({ event = params.event, data = params.data })
elseif params.event == "http_connection_closed" then
    print("Event: " .. params.event)
    core.send_ui_broadcast({ event = params.event, data = params.data })
end
