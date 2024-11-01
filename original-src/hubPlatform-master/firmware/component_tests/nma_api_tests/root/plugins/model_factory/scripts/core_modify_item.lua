local params = ...
    if not params then
        print("Parameters are empty. Expected: item")
        return
    end

local core = require('core')
if not core then
    print('Failed to load core module')
    return
end

core.modify_item( params.item_id, params.item )
