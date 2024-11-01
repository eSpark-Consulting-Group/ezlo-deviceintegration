require "core"
if not core then
    do print( "fail to load the core functionality" ) return end
end

print( "\ncore.add_device..." )

--[[
core.add_device()
core.add_device( {} )
core.add_device( { gateway_id = "zwave_id", default_name = "wassup", type = "linux" } )
core.add_device( { gateway_id = "zwave_id", default_name = "wassup", type = 42 } ) 
core.add_device( { gateway_id = "", default_name = "wassup", type = "int", device_type_id = "that outlet" } ) ]]

local device_id = core.add_device( { type = "linux", default_name = "is", gateway_id = "the best :)",
                                     device_type_id = "some typik",
                                     info = { access = "no" }, battery_powered = true, complex = false } )
print( "device id: ", device_id )

print( "\ncore.get_devices..." )

local devs = core.get_devices()
if devs then
    print( "devs array: ", devs, "size: " .. #devs )
    if devs[1] then
        print( "devs[1].type: ", devs[1].type )
        print( "devs[1].complex: ", devs[1].complex )
    end
end
