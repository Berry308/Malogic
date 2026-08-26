local M = UnLua.Class()

local PlayerServiceName = "Player"

function M:ReceiveManagerInitialized()
    local PlayerService = UE.NewObject(UE.UPlayerVMService, self)
    if not PlayerService or not self:RegisterService(PlayerServiceName, PlayerService) then
        return
    end
end

function M:ReceivePlayerControllerChanged(NewPlayerController)
    local PlayerService = self:FindService(PlayerServiceName)
    if PlayerService then
        PlayerService:SetPlayerController(NewPlayerController)
    end
end

function M:ReceiveManagerDeinitialized()
end

return M
