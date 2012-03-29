local type = type
local pairs = pairs
local tostring = tostring

module ("tableUtils")

local function dump(o,depth)
   if depth==nil then depth=2 end
   if type(o) == "table" then
      local s = "{\n"
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = ""..k.."" end
	 for i=0,depth do
	    s = s .. ' '
	 end
         s = s .. '[' ..k..'] = ' .. dump(v,depth+2) .. ",\n"
      end
      for i=0,depth-2 do
	 s = s .. ' '
      end
      return s..'}'
   elseif type(o) == nil then
      return "<nil>"
   else
      return '"' .. tostring(o):gsub('"', '\"') .. '"' -- lame attempt at proper quoting/escaping
   end
end

local methods = { dump = dump,
	       };

return methods



