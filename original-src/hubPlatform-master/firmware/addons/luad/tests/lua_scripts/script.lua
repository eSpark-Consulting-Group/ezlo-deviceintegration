--send an HTTP GET request
local fail, content, http_status_code = inet.wget( "https://en.wikipedia.org/wiki/Linux", nill, "sergs", "1234" )

print( "result (zero is success): ", fail )
print( "http_status_code: ", http_status_code )
print( content )

--send an HTTTP POST request
local curl_err, response, http_status_code = inet.request( "https://httpbin.org/post", nil, nil, nil, { ["Linux"] = "modest", ["Windows"] = "evil", ["macOS"] = "greedy" } )

print( "curl eror code: ", curl_err )
print( "response to a POST request: ", response )
print( "http_status_code: ", http_status_code )

--send an HTTTP GET request
local curl_err, response, http_status_code = inet.request( "https://httpbin.org/get", nil, nil, nil )

print( "curl eror code: ", curl_err )
print( "response to a GET request: ", response )
print( "http_status_code: ", http_status_code )

--send an HTTTP POST request
local curl_err, response, http_status_code = inet.request( { url = "https://httpbin.org/post",
                                                             data = { ["company"] = "eZLO" },
                                                             headers = { Accept = "application/json" },
                                                             follow = false
                                                           } )

print( "curl eror code: ", curl_err )
print( "response to a POST request: ", response )
print( "http_status_code: ", http_status_code )

--open a connection to ezlo.com and send some string
io.open( 0, "ezlo.com", 443 )

local res = io.is_connected( 0 )
print( "device connection status: ", res )

print( "send data to the server..." )

local res = io.write( "wassup cruel world!" )
if res then
    print( "data has been successfuly sent to the connected server" )
else
    print( "some error happend" )
end

local res = io.is_connected( 0 )
print( "device connection status: ", res )
