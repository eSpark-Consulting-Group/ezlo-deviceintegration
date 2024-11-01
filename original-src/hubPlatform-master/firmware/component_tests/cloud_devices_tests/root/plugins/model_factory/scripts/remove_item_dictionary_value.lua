print( "Remove Dictionary Value" )

local params = ...
if not params then
    print("Parameters is empty")
    return
end

local core = require "core"

local item_id = params.item_id
local key = params.key

if not item_id then
    print("item_id is empty")
    return
end

if not key then
    print("key is empty")
    return
end

core.remove_item_dictionary_value( item_id, key )