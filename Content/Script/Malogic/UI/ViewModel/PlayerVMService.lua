local M = UnLua.Class()
local PlayerStateVMName = "PlayerState"

-- UnLua calls this while the UObject still has RF_NeedInitialization.
function M:Initialize()
    self.BoundPlayerController = nil
    self.BoundPawn = nil
    self.HealthComponent = nil
    self.MagicComponent = nil

    self.PendingPlayerStateViewModel = UE.NewObject(UE.UVMPlayerState, self)
end

-- Called by C++ after the service has been registered and UObject initialization is complete.
function M:ReceiveServiceInitialized()
    local ViewModel = self.PendingPlayerStateViewModel
    self.PendingPlayerStateViewModel = nil

    if ViewModel then
        self:RegisterViewModel(PlayerStateVMName, ViewModel)
    end
end

function M:Normalize(CurrentValue, MaxValue)
    if not MaxValue or MaxValue <= 0 then
        return 0
    end

    return math.max(0, math.min(CurrentValue / MaxValue, 1))
end

function M:ReceivePlayerControllerChanged(NewPlayerController)
    self:UnbindControllerDelegates()
    self:UnbindComponentDelegates()

    self.BoundPlayerController = NewPlayerController
    self.BoundPawn = nil

    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if ViewModel then
        ViewModel:Reset()
    end

    if not NewPlayerController then
        return
    end

    NewPlayerController.OnMalogicPlayerStateChanged:Add(self, M.OnPlayerStateChanged)
    NewPlayerController.OnPossessedPawnChanged:Add(self, M.OnPossessedPawnChanged)
    self:RefreshPawnBindings()
end

function M:ReceiveServiceDeinitialized()
    self:UnbindControllerDelegates()
    self:UnbindComponentDelegates()

    self.PendingPlayerStateViewModel = nil
    self.BoundPlayerController = nil
    self.BoundPawn = nil
    self.HealthComponent = nil
    self.MagicComponent = nil
end

function M:UnbindControllerDelegates()
    local PlayerController = self.BoundPlayerController
    if not PlayerController then
        return
    end

    PlayerController.OnMalogicPlayerStateChanged:Remove(self, M.OnPlayerStateChanged)
    PlayerController.OnPossessedPawnChanged:Remove(self, M.OnPossessedPawnChanged)
end

function M:UnbindComponentDelegates()
    local HealthComponent = self.HealthComponent
    if HealthComponent then
        HealthComponent.OnHealthChanged:Remove(self, M.OnHealthChanged)
        HealthComponent.OnMaxHealthChanged:Remove(self, M.OnMaxHealthChanged)
        HealthComponent.OnDeathStarted:Remove(self, M.OnDeathStarted)
        HealthComponent.OnDeathFinished:Remove(self, M.OnDeathFinished)
    end

    local MagicComponent = self.MagicComponent
    if MagicComponent then
        MagicComponent.OnMagicValueChanged:Remove(self, M.OnMagicValueChanged)
        MagicComponent.OnMaxMagicValueChanged:Remove(self, M.OnMaxMagicValueChanged)
    end

    self.HealthComponent = nil
    self.MagicComponent = nil
end

function M:RefreshPawnBindings(PawnOverride)
    self:UnbindComponentDelegates()

    local PlayerController = self.BoundPlayerController
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if not PlayerController or not ViewModel then
        return
    end

    ViewModel:Reset()

    local Pawn = PawnOverride or PlayerController:K2_GetPawn()
    self.BoundPawn = Pawn
    if not Pawn then
        return
    end

    self:BindHealthComponent(UE.UMalogicHealthComponent.FindHealthComponent(Pawn))
    self:BindMagicComponent(UE.UMalogicMagicComponent.FindMagicComponent(Pawn))
end

function M:BindHealthComponent(HealthComponent)
    if not HealthComponent then
        return
    end

    self.HealthComponent = HealthComponent
    HealthComponent.OnHealthChanged:Add(self, M.OnHealthChanged)
    HealthComponent.OnMaxHealthChanged:Add(self, M.OnMaxHealthChanged)
    HealthComponent.OnDeathStarted:Add(self, M.OnDeathStarted)
    HealthComponent.OnDeathFinished:Add(self, M.OnDeathFinished)

    local ViewModel = self:FindViewModel(PlayerStateVMName)
    local Health = HealthComponent:GetHealth()
    local MaxHealth = HealthComponent:GetMaxHealth()
    ViewModel:SetHealth(Health)
    ViewModel:SetMaxHealth(MaxHealth)
    ViewModel:SetHealthNormalized(self:Normalize(Health, MaxHealth))
    ViewModel:SetDeathState(HealthComponent:GetDeathState())
end

function M:BindMagicComponent(MagicComponent)
    if not MagicComponent then
        return
    end

    self.MagicComponent = MagicComponent
    MagicComponent.OnMagicValueChanged:Add(self, M.OnMagicValueChanged)
    MagicComponent.OnMaxMagicValueChanged:Add(self, M.OnMaxMagicValueChanged)

    local ViewModel = self:FindViewModel(PlayerStateVMName)
    local MagicValue = MagicComponent:GetMagicValue()
    local MaxMagicValue = MagicComponent:GetMaxMagicValue()
    ViewModel:SetMagicValue(MagicValue)
    ViewModel:SetMaxMagicValue(MaxMagicValue)
    ViewModel:SetMagicNormalized(self:Normalize(MagicValue, MaxMagicValue))
end

function M:OnPlayerStateChanged()
    self:RefreshPawnBindings()
end

function M:OnPossessedPawnChanged(OldPawn, NewPawn)
    self:RefreshPawnBindings(NewPawn)
end

function M:OnHealthChanged(HealthComponent, OldValue, NewValue, Instigator)
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if HealthComponent == self.HealthComponent and ViewModel then
        ViewModel:SetHealth(NewValue)
        ViewModel:SetHealthNormalized(self:Normalize(NewValue, HealthComponent:GetMaxHealth()))
    end
end

function M:OnMaxHealthChanged(HealthComponent, OldValue, NewValue, Instigator)
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if HealthComponent == self.HealthComponent and ViewModel then
        ViewModel:SetMaxHealth(NewValue)
        ViewModel:SetHealthNormalized(self:Normalize(HealthComponent:GetHealth(), NewValue))
    end
end

function M:OnDeathStarted(OwningActor)
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if OwningActor == self.BoundPawn and ViewModel then
        ViewModel:SetDeathState(UE.EMalogicDeathState.DeathStarted)
    end
end

function M:OnDeathFinished(OwningActor)
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if OwningActor == self.BoundPawn and ViewModel then
        ViewModel:SetDeathState(UE.EMalogicDeathState.DeathFinished)
    end
end

function M:OnMagicValueChanged(MagicComponent, OldValue, NewValue, Instigator)
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if MagicComponent == self.MagicComponent and ViewModel then
        ViewModel:SetMagicValue(NewValue)
        ViewModel:SetMagicNormalized(self:Normalize(NewValue, MagicComponent:GetMaxMagicValue()))
    end
end

function M:OnMaxMagicValueChanged(MagicComponent, OldValue, NewValue, Instigator)
    local ViewModel = self:FindViewModel(PlayerStateVMName)
    if MagicComponent == self.MagicComponent and ViewModel then
        ViewModel:SetMaxMagicValue(NewValue)
        ViewModel:SetMagicNormalized(self:Normalize(MagicComponent:GetMagicValue(), NewValue))
    end
end

return M
