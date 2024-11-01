local params = ...

assert(type(params) == 'table', 'Invalid argument type, table required')
local logger = require('logging')
local core = require('core')

local function make_connection_key( id )
    return 'connection_id' .. (id or '')
end

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

local charset = {}

for i = 32,  57 do table.insert(charset, string.char(i)) end
for i = 65,  90 do table.insert(charset, string.char(i)) end
for i = 97, 122 do table.insert(charset, string.char(i)) end

function string.random(length)
    math.randomseed(os.time()/64596 )

    if length <= 0 then return "" end

    local result = {}
    for _ = 1,length do
        table.insert( result, charset[math.random(1, #charset)] )
    end
    return table.concat(result)
end

print('Event received: ' .. params.event)
if params.event == 'http_data_received' then
    print('Dump params.data: ')
    if #params.data.data > 200 then
        print(params.event .. ' with lots of data received: ' .. tostring(#params.data.data))
        for k, v in pairs(params.data) do
            if k ~= 'data' then
                print(params.event .. '.' .. k .. ': ' .. tostring(v))
            end
        end
    else
        dump(params.data, 'http_data_received')
    end
    core.send_ui_broadcast(
            {
                event = params.event,
                event_data = params.data
            }
    )
elseif params.event == 'http_data_send' then
    print('Dump params.data: ')
    dump(params.data, 'http_data_send')

    local message = 'Data appended in http_data_send handler'
    local message_size = string.len(message)
    local http_server = require('http_server')

    -- get connection params, check params.random_data_size
    local conn_info = http_server.get_connection_info( params.data.id )
    dump(conn_info, "conn_info")
    local wait_before_send_data = 0
    local max_portion_size = 32000
    local use_broadcasts = 'yes'
    local random_data_size = 0

    local params_index = {}
    if conn_info['params'] ~= nil and type(conn_info.params) == 'table' and #conn_info.params > 0 then
        for _,obj in ipairs( conn_info.params ) do
            for key,val in pairs(obj) do
                params_index[key] = val
            end
        end

        max_portion_size = tonumber(params_index.portion or '0')
        random_data_size = tonumber( params_index.random_data_size or '0' )
        if random_data_size > 0 then
            print('Random string of size generated: ' .. tostring(random_data_size) )
            local size = math.min( random_data_size, max_portion_size )
            message = string.random( size )
            message_size = size
        end

        -- TODO: if params.random_data_size > max_portion_size and params.portion == max_portion_size,
        --      use last=false to stream data to the server

        local status_ok, value = pcall( tonumber, params_index.wait_before_send_data or 0 )
        if status_ok then
            wait_before_send_data = value
        end

        use_broadcasts = params_index.use_broadcasts or 'yes'
    end

    if (params_index.action or '') == 'connection_info' then
        logger.trace( 'Action: ' .. params_index.action )
        local result = http_server.get_connection_info(params.data.id)
        core.send_ui_broadcast( { action = "connection_info", action_data = result } )
    elseif  (params_index.action or '') == 'close_connection' then
        logger.trace( 'Action: ' .. params_index.action )
        local result = http_server.close_connection(params.data.id)
        core.send_ui_broadcast( { action = "close_connection", action_data = result } )
    end

    if wait_before_send_data == 0 or (params.callback or false) == true then
        local send_data_params = {
            id = params.data.id,
            data = message,
            headers = { { ['content-type'] = "text/plain" }, { ["content-length"] = tostring( math.max(random_data_size, message_size) ) } },
            last = true
        }
        -- set custom code and status if passed
        if params_index.http_code ~= nil then
            send_data_params.code = tonumber( params_index.http_code )
            if params_index.http_status ~= nil then
                send_data_params.code_status = params_index.http_status
            end
        end

        print( 'type(max_portion_size)', type(max_portion_size) )
        if max_portion_size > 0 and max_portion_size < math.max(random_data_size, message_size) then
            local connection_key = make_connection_key(params.data.id)
            local storage = require('storage')

            local position = 1
            local prev_send_data = {}
            if storage.exists( connection_key ) then
                prev_send_data = storage.get_table(connection_key)
                position = prev_send_data.position

                local bytes_left_in_portion = math.min(random_data_size - position, max_portion_size)

                send_data_params.data = string.sub(message, 1, bytes_left_in_portion)
                position = position + string.len(send_data_params.data)
                print( 'storage.exists() = true: position calculated: ' .. tostring(position))
                send_data_params.last = position >= math.max(message_size, random_data_size)
            else
                position = 0
                send_data_params.data = string.sub(message, 1, max_portion_size)
                send_data_params.last = false
                position = string.len(send_data_params.data)
                print( 'storage.exists() = false: position calculated: ' .. tostring(position))
            end

            local storage_record = {
                last = send_data_params.last,
                portion_size = string.len(send_data_params.data),
                position = position,
                total_size = string.len(message)
            }
            print('Save to storage: ' .. connection_key )
            dump( storage_record, connection_key )

            storage.set_table( connection_key, storage_record )
        end

        local send_data_result = http_server.send_data(send_data_params)

        local data_size = string.len(send_data_params.data)
        if data_size > 100 then
            send_data_params.data = 'Truncated, size=' .. tostring(data_size)
        end
        dump( send_data_params, 'send_data(params), truncated')

        if use_broadcasts == 'yes' then
            core.send_ui_broadcast(
                    {
                        event = params.event,
                        event_data = params.data,
                        send_data_result = send_data_result,
                        connection_id = params.data.id,
                        handler_data = send_data_params
                    }
            )
        end
    else
        print('Creating timer for 1000 ms to call send_data later...');
        require('timer').set_timeout(
                wait_before_send_data,
                'HUB:http_server_test/scripts/handlers/send_data_handler',
                {
                    event = params.event,
                    data = params.data,
                    callback = true
                } )
    end
elseif params.event == 'http_connection_closed' then
    print('Dump event.data: ')
    dump(params.data, 'http_connection_closed')

    core.send_ui_broadcast(
            {
                event = params.event,
                event_data = params.data,
            }
    )

    local connection_key = make_connection_key( params.data.id )
    require('storage').delete( connection_key )
end