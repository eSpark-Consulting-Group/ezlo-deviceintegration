local params = ...
if not params then
    print("Parameters are empty. Expected http event data.")
    return
end

local http = require('http')
local core = require('core')

--local newData = "Data to send from lua repeat_request event handler"
local eventData = params.data

function dump( arg, prefix )
    prefix = prefix or 'empty'
    -- print("Dump called: " .. prefix)
    if type(arg) == 'table' then
        for k,v in pairs(arg) do
            -- print( (prefix or '') .. '.' .. k .. ', typeof: ' .. type(v) )
            dump(v, prefix .. '.' .. k)
        end
    else
        print(prefix .. ': ' .. tostring(arg))
    end
end

dump(params, 'params')

print("Event: " .. (params.event or 'unknown_event') .. ', connection id: ' .. eventData.id .. ', url: ' .. eventData.url)

if params.event == "http_data_received" then
    core.send_ui_broadcast({ event = params.event, data = params.data })
    if eventData.last then
        local nextHandler = "HUB:http_test/scripts/http_events"
        local nextData = eventData.user_data
        local status, errOrCounter = pcall(tonumber, eventData.user_data)
        if status and errOrCounter then
            print( errOrCounter )
            nextData = tostring(errOrCounter + 1)
            if errOrCounter + 1 < 5 then
                -- exit loop here
                nextHandler = "HUB:http_test/scripts/http_repeat_request"
            end
        end

        -- Send next request and exit this loop by redirecting to http_events
        local request_data = "Data from http_repeat_request.lua script, user_data: " .. nextData
        local nextStep = {
            id = eventData.id,
            user_data = nextData,
            keep_alive = true,
            type = 'POST',
            handler = nextHandler,
            timeout = 5000,
            data = request_data,
            -- hands up without content_length property
            content_length = #request_data
        }

        print('http.repeat_request: -> ')
        print({ next_step = nextStep })
        local status, err_or_value = pcall(http.repeat_request, nextStep)
        if not status then
            nextStep.error = err_or_value
        end

        nextStep.source = "http_repeat_request event handler"
        core.send_ui_broadcast(nextStep)
    else
        local msg = "Not a last portion, skip."
        print(msg)
    end
elseif params.event == "http_connection_closed" then
    print("Event: " .. params.event)
    core.send_ui_broadcast({ event = params.event, data = params.data })
end

