print( "item_update handler" )

local params = ...
local core = require "core"

local test = params[ "testSelector" ]
local item_id = params[ "itemId" ]
local item_ids = params[ "itemIds" ]
local item_value = params[ "itemValue" ]
local dictionary_key = params[ "dictionaryKey" ]
local source = params[ "source" ]

if test == "set_item_value" then
    if source == nil then
        core.set_item_value( item_id, item_value )
    else
        core.set_item_value( item_id, item_value, source )
    end
elseif test == "set_item_value_in_batch" then
    if source == nil then
        core.set_item_value_in_batch( item_ids, item_value )
    else
        core.set_item_value_in_batch( item_ids, item_value, source )
    end
elseif test == "add_item_dictionary_value" then
    if source == nil then
        core.add_item_dictionary_value( item_id, item_value )
    else
        core.add_item_dictionary_value( item_id, item_value, source )
    end
elseif test == "set_item_dictionary_value" then
    if source == nil then
        core.set_item_dictionary_value( item_id, dictionary_key, item_value )
    else
        core.set_item_dictionary_value( item_id, dictionary_key, item_value, source )
    end
elseif test == "delete_item_dictionary_value" then
    if source == nil then
        core.delete_item_dictionary_value( item_id, dictionary_key )
    else
        core.delete_item_dictionary_value( item_id, dictionary_key, source )
    end
end


