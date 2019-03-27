--
-- parse a http header (example)
--

data = require'data'

local parse = {}

-- check if the http message is somewhat sane
function parse.sanitycheck(s)
   -- line breaks
   if not string.find(s, "\r\n") then return end
   return true
end

-- parses a GET request
function parse.requestmethod(s)
   local ok = string.match(s, "^GET")
   if not ok then return end
   local path = string.match(s, "GET (%g+) HTTP")
   --print(path)
   return path
end

-- parses HTTP fields
function parse.requestfields(s, header)
   local size = 0
   -- we won't parse any fields that are fragmented
   for k, v in string.gmatch(s, "(%g*):%s*([^%c]+)") do
      header[k] = v
      size = size + 1
   end
   return size ~= 0, size
end

function parse.http(s)
   local header = {}
   if parse.sanitycheck(s) then
      header.GET = parse.requestmethod(s)
      local ok, size = parse.requestfields(s, header)
      if not ok then return end
      header.__size = size
      return true, header
   end
   return
end

function process(buff)
   local buff = buff:layout{str = {0, #buff, "string"}}

   local ok, header = parse.http(buff.str)

   if ok then
      return true
   else
      print("Rejected buf:")
      print(buff)
      print("\n")
   end
end
