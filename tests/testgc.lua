--
-- gc acts here constantly
--

function process()
   local tb = {}
   for i = 1, 131072 do
      tb[i] = i
   end
   return false
end
