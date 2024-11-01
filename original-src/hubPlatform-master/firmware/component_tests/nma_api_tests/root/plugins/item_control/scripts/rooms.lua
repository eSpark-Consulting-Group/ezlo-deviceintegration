local params = ... or {}
local core = require 'core'

if params.test == 'get_rooms' then
    local rooms_list = core.get_rooms()
    if params.expect_size ~= nil and type(params.expect_size) == 'number' then
        if rooms_list == nil then
            if params.expect_size ~= 0 then
                error('Rooms list is empty')
            end

            return
        end

        if params.expect_size ~= #rooms_list then
            error('Expected size does not match')
        end
    end
elseif params.test == 'get_room' then
    local room = core.get_room(params.room_id)
    if room == nil then
        error('Room not found by id: ' .. params.room_id)
    end
elseif params.test == 'help' then
    print('rooms.lua test=test_name[, ...]')
    print('Tests:')
    print('    get_rooms [expect_size=number]- get list of all existing rooms')
    print('    get_room [room_id=string]- get a room by id')
end