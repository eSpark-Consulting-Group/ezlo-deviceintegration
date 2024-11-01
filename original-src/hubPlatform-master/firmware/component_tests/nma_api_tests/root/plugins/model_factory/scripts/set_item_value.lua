print( "Set Value" )

local params = ...
local core = require "core"
print( core )

if params.item_ids then
    local i = 0
    for _ in pairs( params.item_ids ) do
        i = i + 1
        core.update_item_value( params.item_ids[i], params.value )
    end
elseif params.items then
    for k, v in pairs(params.items) do
        print( k )
        core.update_item_value( k, v.value )
    end
else
    core.update_item_value( params.item_id, params.value )
end
