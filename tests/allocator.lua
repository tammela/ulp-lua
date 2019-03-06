--
-- we are basically testing all possible calls to the allocator.
-- this small code tests reallocations, frees and mallocs.
--

function process()
   local tb = {}
   for i = 1, 16777216 do
      tb[i] = i
   end
   return false
end
