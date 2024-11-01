local params = ...
    if not params then
        print("Parameters are empty.")
        return
    end

local core = require('core')
if not core then
    print('Failed to load core module')
    return
end

local status = core.get_feature_status( params.feature_name)

core.send_ui_broadcast( {status = status} )