
newoption {
    trigger = "sharkdir",
    value = "path",
    description = "Create Solution with different Engine"
}

SharkDir = nil

if (_OPTIONS["sharkdir"]) then
    SharkDir = _OPTIONS["sharkdir"]
    print("SharkDir: " .. SharkDir)
end

if (SharkDir == nil) then
    error("No SharkDir specified!")
end

if not (os.isdir(SharkDir)) then
    error("SharkDir isn't a directory")
end

if not (os.isdir(path.join(SharkDir, "ScriptingCore"))) then
    error("Couldn't locate ScriptingCore in SharkDir!")
end
