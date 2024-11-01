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
    print('Dump params.data: ')
    dump(params.data, 'http_data_received')
elseif params.event == 'http_data_send' then
    print('Dump params.data: ')
    dump(params.data, 'http_data_send')
    local http_server = require('http_server')
    local connection_info_result = http_server.get_connection_info(params.data.id)
    dump( connection_info_result, "connection_info_result" )

    local close_result = http_server.close_connection(params.data.id)

    require('core').send_ui_broadcast({
                close_result = close_result,
                connection_id = params.data.id
    })
elseif params.event == 'http_connection_closed' then
    print('Dump event.data: ')
    dump(params.data, 'http_connection_closed')
end
