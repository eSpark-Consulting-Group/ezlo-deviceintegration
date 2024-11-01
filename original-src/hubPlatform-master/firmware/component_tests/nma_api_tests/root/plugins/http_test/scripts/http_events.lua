local params = ...
if not params then
    print("Parameters are empty. Expected http event data.")
    return
end

local core = require('core')

core.send_ui_broadcast(params)