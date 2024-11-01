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

local result = core.add_item( params.item )
if not result then
    error("Item adding error")
end
