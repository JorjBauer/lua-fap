#!/usr/local/bin/lua

require "fap"
local tu = require "tableUtils"

print("Testing Fabulous APRS Parser library interface...")

fap.init()

s="AB3AG-7>TP0U2Y,N3XKU-15*,WIDE2-2,qAR,KA1UDX-1:'ga/l  >/>testing antennas"

print (tu.dump(fap.parseaprs(s)))

fap.cleanup()

