_G._funcScriptCodeTick_ = _G._funcScriptCodeTick_ or {}
local funcTick = _G._funcScriptCodeTick_
-------------------------------------------------------
local _scriptname_ = [=[
Kịch bản]=]
local _luascriptstr_ = [==[
Chat:sendSystemMsg("hello")
]==]
local _modpacketid_ = nil
local f = DoLuaScript(_luascriptstr_, _scriptname_, _mod_, _modpacketid_)
if f then
    table.insert(funcTick, {
        func = f,
        title = _scriptname_,
    })
end
