print( "item_update handler" )

local params = ...
local core = require "core"

local test = params[ "testSelector" ]
local item_id = params[ "itemId" ]
local dev_id = params[ "deviceId" ]

function big_string()
    str = ""
    for a=1,20000 do
      str = str .. tostring(a)
      str = str .. " "
    end
    return str
end

function add_device()
    local gateway = core.get_gateway()
    local dev_id = core.add_device( { gateway_id = gateway.id,
                                     name = "testDevice1",
                                     type = "sensor",
                                     device_type_id = "security_sensor",
                                     category = "security_sensor",
                                     battery_powered = false } )
    return dev_id
end

function create_item(dev_id, value)
   local item = { device_id = dev_id,
                  name = "testItem1",
                  value_type = "string",
                  has_getter = true,
                  has_setter = false,
                  value = value }
   return item
end

if test == "set_item_value" then
        core.set_item_value( item_id, big_string() )
end

-- if test == "modify_item" then
--     local gateway = core.get_gateway()
--            dev_id = add_device()
--            if dev_id then
--                 local item = create_item(dev_id, "")
--                 item_id = core.add_item( item )
--                 item.value = big_string()
--                 --core.modify_item(item_id, item)
--
--                 core.modify_item( item["_id"], item )
--            end
-- end

if test == "add_item" then
       local gateway = core.get_gateway()
       dev_id = add_device()
       if dev_id then
            local item = create_item(dev_id, big_string())
            core.add_item( item )
       end
end
