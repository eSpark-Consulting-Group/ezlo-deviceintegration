print( "restricted_methods handler" )

local params = ...
local core = require "core"
local devices = core.get_devices()
local items = core.get_items()
local device = devices[1]
local item = items[1]

device[ "gateway_id" ] = device[ "gatewayId" ]
device[ "device_type_id" ] = device[ "deviceTypeId" ]
device[ "battery_powered" ] = device[ "batteryPowered" ]
device[ "parent_room" ] = device[ "parentRoom" ]

item[ "device_id" ] = item[ "deviceId" ]
item[ "value_type" ] = item[ "type" ]
item[ "has_getter" ] = item[ "hasGetter" ]
item[ "has_setter" ] = item[ "hasSetter" ] 
item[ "enum" ] = nil

local test = params[ "testSelector" ]

if test == "add_device" then
    core.add_device( device )
elseif test == "modify_device" then
    core.modify_device( device["_id"], device )
elseif test == "add_item" then
    core.add_item( item )
elseif test == "modify_item" then
    core.modify_item( item["_id"], item )
elseif test == "remove_device_sync" then
    core.remove_device_sync( device["_id"] )
elseif test == "remove_device" then
    core.remove_device( device["_id"] )
elseif test == "remove_gateway_devices" then
    core.remove_gateway_devices( device["gatewayId"] )
elseif test == "update_device_status" then
    core.update_device_status( device["_id"], "good" )
elseif test == "update_item_value_with_min_max" then
    core.update_item_value_with_min_max( item["_id"], 1, 0, 10 )
elseif test == "update_reachable_state" then
    core.update_reachable_state( device["_id"], false )
elseif test == "update_ready_state" then
    core.update_ready_state( device["_id"], false )
elseif test == "update_item_value" then
    core.update_item_value( item["_id"], false )
elseif test == "update_item_dictionary_value" then
    core.update_item_dictionary_value( item["_id"], "2", 2 )
elseif test == "remove_item_dictionary_value" then
    core.remove_item_dictionary_value( item["_id"], "2" )
end
