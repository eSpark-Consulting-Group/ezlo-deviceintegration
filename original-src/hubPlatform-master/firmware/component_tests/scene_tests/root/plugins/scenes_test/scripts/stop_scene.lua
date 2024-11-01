local scenes = require "scenes"
local core = require "core"

local params = ...
--
if params[ "scene_id" ] == nil then
    core.send_ui_broadcast({ data = { error = "The scene with this id does not exist" } })
else
    local scene_id = params[ "scene_id" ]
    if params[ "block" ] == nil and params[ "group" ] == nil then
        local status, error = pcall( scenes.stop_scene, scene_id )
        if not status then
            print("Error: " .. error)
            core.send_ui_broadcast({ error = { data = error } })
        end
    elseif params[ "block" ] ~= nil and params[ "group" ] ~= nil then
        local group = params[ "group" ]
        local block = params[ "block" ]
        local status, error = pcall(scenes.stop_scene, scene_id, block, group)
        if not status then
            print("Error: " .. error)
            core.send_ui_broadcast({ error = { data = error } })
        end
    elseif params[ "block" ] ~= nil and params[ "group" ] == nil then
        local block = params[ "block" ]
        local status, error = pcall(scenes.stop_scene, scene_id, block)
        if not status then
            print("Error: " .. error)
            core.send_ui_broadcast({ error = { data = error } })
        end
    else
        print("Error: Wrong params")
        core.send_ui_broadcast({ data = { error = "Wrong params" } })
    end
end