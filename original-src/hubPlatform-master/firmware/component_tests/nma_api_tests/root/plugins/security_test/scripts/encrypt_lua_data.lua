local security = require("security")
local logger = require("logging")

local function StringToHex(str) -- For debugging purposes
    local t = {}
    for i in str:gmatch "." do
        local c = string.format("\\x%02x", string.byte(i))
        if i:match("%p") or i:match("%w") or i:match("%s") then
            table.insert(t, i)
        else
            table.insert(t, c)
        end
    end
    return table.concat(t, "")
end

local IK = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02" -- initial hex key
local IV = "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58" -- initial hex vector

local packet =
    "\x00\x00\x00\x00111111111111111\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00Test  1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

logger.debug(StringToHex(packet))
local enc_packet =
    security.encrypt(
    {
        key = IK,
        type = "aes",
        mode = "cbc",
        iv = IV,
        auto_padding = false
    },
    packet
)

logger.debug("Enc payload: " .. StringToHex(enc_packet))
logger.debug("Encrypted payload length: " .. #enc_packet)

local core = require('core')
if not core then
    print('Failed to load core module')
    return
end

local params = { value=enc_packet }

core.send_ui_broadcast( params )
