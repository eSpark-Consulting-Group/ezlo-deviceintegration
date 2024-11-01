local params = ...
local house_modes = assert(require("house_modes"), "Failed to load security module")
local core = assert(require("core"), "Fail to load core functionality")

local result = house_modes.subscribe("HUB:house_modes_test/scripts/event_handler")
params.id = result
print("house_modes.subscribe: -> ")

core.send_ui_broadcast(params)