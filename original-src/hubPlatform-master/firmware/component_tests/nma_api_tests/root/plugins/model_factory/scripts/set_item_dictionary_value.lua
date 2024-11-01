print( "Set Dictionary Value" )

local params = ...
if not params then
    print("Parameters is empty")
    return
end

local core = require "core"

local item_id = params.item_id
local value = params.value
local key = params.key

if not key then
    key = tostring(core.generate_item_dictionary_number_id(
        item_id, 
        1,
        1000
    ))
end

core.update_item_dictionary_value( item_id, key, value )