print( "item_update handler" )

local params = ...
local core = require "core"

local test = params[ "testSelector" ]
local item_id = params[ "itemId" ]
local item_value = params[ "itemValue" ]
local dictionary_key = params[ "dictionaryKey" ]

if test == "set_item_value" then
    core.set_item_value( item_id, item_value )
elseif test == "add_item_dictionary_value" then
    core.add_item_dictionary_value( item_id, item_value )
elseif test == "set_item_dictionary_value" then
    core.set_item_dictionary_value( item_id, dictionary_key, item_value )
elseif test == "delete_item_dictionary_value" then
    core.delete_item_dictionary_value( item_id, dictionary_key )
end


