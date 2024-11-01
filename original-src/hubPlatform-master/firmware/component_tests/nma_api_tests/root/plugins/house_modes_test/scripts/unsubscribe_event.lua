local params = ...
local house_modes = assert(require("house_modes"), "Failed to load security module")
local core = assert(require("core"), "Fail to load core functionality")

house_modes.unsubscribe("HUB:house_modes_test/scripts/event_handler")
print("house_modes.unsubscribe: -> ")

core.send_ui_broadcast(params)