local M = UnLua.Class()

function M:ReceiveOnAbilityActivated()
    print("ReceiveOnAbilityActivated")

    self:CharacterJumpStart()

    -- 开启一个命名状态
    local jumpState = UE.UAbilityTask_StartAbilityState.StartAbilityState(
        self,
        "Jumping",
        true
    )
    -- 优势：你可以非常清晰地处理各种异常退出
    jumpState.OnStateInterrupted:Add(self, M.OnStateInterrupted)
    jumpState.OnStateEnded:Add(self, M.OnStateEnded)
    jumpState:ReadyForActivation()

    local InputState = UE.UAbilityTask_WaitInputRelease.WaitInputRelease(self,false)
    InputState.OnRelease:Add(self,M.OnInputReleased)
    InputState:ReadyForActivation()
end

function M:OnStateInterrupted()
    print("Jump state interrupted")
    self:CharacterJumpStop()
end

function M:OnStateEnded()
    -- print("Jump state ended")
    self:CharacterJumpStop()
end

function M:OnInputReleased()
    -- print("Jump input released")
    self:OnInputReleasedLua()
end

return M