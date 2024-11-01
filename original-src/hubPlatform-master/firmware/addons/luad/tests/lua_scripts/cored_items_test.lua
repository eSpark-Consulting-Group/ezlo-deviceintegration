require "core"
if not core then
    do print( "fail to load the core functionality" ) return end
end

print( "\ncore.add_item..." )

--[[
core.add_item()
core.add_item( {} )
core.add_item( { name = "" } )
core.add_item( { name = "hi", value_type = 42 } )
core.add_item( { name = "hi", value_type = "intA" } )
core.add_item( { name = "hi", value_type = "int", sub_type = 42 } ) ]]

local item_id = core.add_item( { device_id = "1", name = "switch", value_type = "int", 
    has_getter = true, has_setter = false, battery_powered = true, reachable = true } )
print( "item id: ", item_id )

print( "\ncore.get_items..." )

local items = core.get_items()
if items then
    print( "items array: ", items, "size: " .. #items )
    if items[1] then
        print( "items[1].name: ", items[1].name )
        print( "items[1].type: ", items[1].type )
        print( "items[1].hasGetter: ", items[1].hasGetter )
    end
end
