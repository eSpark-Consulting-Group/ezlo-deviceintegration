local params = ...

assert(type(params) == 'table', 'Invalid argument type, table required')

local function dump(arg, prefix)
    prefix = prefix or 'empty'
    if type(arg) == 'table' then
        for k, v in pairs(arg) do
            dump(v, prefix .. '.' .. k)
        end
    else
        print(prefix .. ': ' .. tostring(arg))
    end
end

print('Event received: ' .. params.event)
if params.event == 'http_data_received' then
    local conn_id = params.data.id
    local http_server = require('http_server')

    local event_data = params.data;
    event_data.data_md5 = require('security').md5( event_data.data or '' );
    if string.len(event_data.data) > 1000 then
        local data_size = string.len(event_data.data)
        event_data.data = 'Truncated : data_size=' .. tostring(data_size)
        event_data.data_size = data_size
    end
    print('Dump params.data: ')
    dump(event_data, 'http_data_received (truncated)')
    require('core').send_ui_broadcast(
            {
                event = params.event,
                event_data = event_data,
            }
    )

elseif params.event == 'http_data_send' then
    print('Dump event.data: ')
    dump(params.data, 'http_data_send')
    local portion = {
        id = params.data.id,
        data = 'Array of bytes added in http_data_send event!!!\n',
        last = true,
        code = 200,
        code_status = 'Ok'
    }
    http_server.send_data(portion)

    require('core').send_ui_broadcast(
            {
                event = params.event,
                event_data = params.data,
                handler_data = {
                    portion = portion
                }
            }
    )

elseif params.event == 'http_connection_closed' then
    print('Dump event.data: ')
    dump(params.data, 'http_connection_closed')

    require('core').send_ui_broadcast(
            {
                event = params.event,
                event_data = params.data,
            }
    )
end
