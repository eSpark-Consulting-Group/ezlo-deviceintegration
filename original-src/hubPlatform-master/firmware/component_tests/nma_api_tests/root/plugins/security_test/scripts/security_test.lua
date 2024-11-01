local params = ...
if not params then
    print("Parameters are empty. Expected: hash: [md5 | sha1], data: string, method: string (opt)")
    return
end

local security = require('security')
if not security then
    print('Failed to load security module')
    return
end

local core = require('core')

local value = 'unknown hash'

if params.method ~= nil and params.method == 'digest_by_name' then
    value = security.digest_by_name(params.hash, params.data)
elseif params.method ~= nil and params.method == 'get_all_digest_names' then
    value = security.get_all_digest_names()
elseif params.method ~= nil and params.method == 'from_hex_string' then
    local binary_value = security.from_hex_string(params.data)
    params.binary_value = binary_value
    if params.hash == 'base64' then
        value = security.to_base64(binary_value)
    else
        value = security.digest_by_name(params.hash, binary_value)
    end
elseif (params.hash == 'md5') then
    value = security.md5(params.data)
elseif (params.hash == 'md5_binary') then
    print('Adding zero before parameter: ' .. params.data)
    value = security.md5('\000' .. params.data)
elseif (params.hash == 'sha1') then
    value = security.sha1(params.data)
elseif (params.hash == 'sha256') then
    value = security.sha256(params.data)
end

params.value = value
core.send_ui_broadcast(params)