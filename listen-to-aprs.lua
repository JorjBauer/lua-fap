#!/usr/bin/env lua

-- Load the luarocks loader, if it's available. Uses pcall() to ignore errors.
pcall(require, "luarocks.loader")

local socket = require "socket"
local tu = require "tableUtils"
local fap = require "fap"
require "alarm"

function timeout()
   print("no packets received in 60 seconds -- exiting")
   os.exit(-3)
end

-- expand single spaces in the first 9 characters after the first colon; make
-- them double-spaces. (Crude attempt to work around a problem that is 
-- reputedly introduced by javAPRS.)
function expand(line)
   local first,last = string.find(line, ":")
   local prefix = string.sub(line, 1, first-1)
   local suffix = string.sub(line, last+1)

   local data = string.gsub(string.sub(suffix, 1, 8), " ", "  ")

   local result = prefix .. ":" .. data .. string.sub(suffix, 9)

   return result
end

function main()
   if (#arg ~= 2) then
      print("You need to provide a host and port as arguments")
      os.exit(-4)
   end
   
   host=arg[1]
   port=arg[2]
   
   fap.init()
   
   alarm(60, timeout)
   print("Connecting to " .. host .. ":" .. port)
   c = socket.connect(host, port)
   if (c == nil) then
      print "unable to connect; exiting"
      os.exit(-1)
   end
   
   line = c:receive("*l")
   repeat
      alarm(60, timeout) -- No packets in 60 seconds? Give up.

      if string.sub(line, 1, 1) ~= "#" then
	 local s = fap.parseaprs(line)
	 -- Some packets are mangled, reputedly by javAPRS. If we fail to 
	 -- parse then quickly try to un-mangle.
	 if (s.error_code ~= 0) then
	    line = expand(line)
	    s = fap.parseaprs(line)
	 end

	 if (s.error_code ~= 0) then
	    print("Unable to parse " .. line .. ": " .. s.error_message ..
		  " [" .. s.error_code .. "]")
	 else
	    print (tu.dump(fap.parseaprs(line)))
	 end
      end
      line = c:receive("*l")
   until line==nil
   
   print("Connection dropped; exiting")
   os.exit(-2)
end

main()
os.exit(0)
