
local included = {}
local dst = assert(io.open(arg[1], "w+"))

local function readFile(path)
    local file = assert(io.open("src/".. path, "r"))
    local data = file:read("*a")
    file:close()
    return data
end

local function includeFile(path)
    if included[path] then return "" end
    
    included[path] = true
    
    local str = readFile(path)
    
    str = str:gsub('#%s*include%s*"([^"]+)"[^\n]*\n', includeFile)
    
    return str
end

for i = 2, #arg do
    dst:write(includeFile(arg[i] .. ".c"))
end

dst:close()
