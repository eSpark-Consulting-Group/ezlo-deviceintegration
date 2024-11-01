
--check/get the zwave functionality, get a module/table
local zwave = require "zwave" --note 'require' requires a module to be a global one, so 'local zwave' is redundant 
if not zwave then
    do print( "fail to load the zwave functionality" ) return end 
end

zwave.start_include()

--subscribe to be notified about all zwaved' events and use a specified script for handling those events
zwave.subscribe( "events_handling" )
