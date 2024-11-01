print( "Clear Dictionary" )

local params = ...
if not params then
    print("Parameters is empty")
    return
end

local core = require "core"
core.update_item_value( params.item_id, {} )
