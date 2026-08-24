var renameTargetMode = 'slot';
var tempStars = 3;
var lastMenuOpenTime = 0;
var toastTimeout = null;

var modNameMap = { 0: 'None', 42: 'Shift', 29: 'Ctrl', 56: 'Alt' };
var gpModNameMap = { 0: 'None', 256: 'LB', 512: 'RB', 9: 'LT', 10: 'RT' };

function showSaveToast(text) {
    const toast = document.getElementById('save-toast');
    if (!toast) return;
    toast.innerText = text || "Outfit Saved!";
    toast.classList.add('show');
    if (toastTimeout) clearTimeout(toastTimeout);
    toastTimeout = setTimeout(() => {
        toast.classList.remove('show');
    }, 1500);
}

function hideAllModals() {
    ['rename-modal', 'context-modal', 'settings-modal', 'keyboard-modal', 'gamepad-modal'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    listeningKeyType = null;
    listeningGamepadType = null;
    closeAllDropdowns();
}

function hideAllMenus(e) {
    if (e) e.stopPropagation();
    ['slot-context-menu', 'wheel-context-menu'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    closeAllDropdowns();
}

function showSlotContextMenu(x, y, slotIndex) {
    if (!isEditModeActive()) return;
    selectedSlotIndex = slotIndex;
    hideAllMenus();

    const menu = document.getElementById('slot-context-menu');
    const title = document.getElementById('slot-ctx-title');
    if (!menu || !title) return;

    title.innerText = 'Slot ' + (slotIndex + 1) + ' Options';

    const currentSlot = currentWheelData.slots.find(s => s.index === slotIndex);
    const wardrobeBtn = document.getElementById('ctx-slot-wardrobe-toggle');
    if (wardrobeBtn) {
        if (!currentSlot || !currentSlot.hasOutfit) {
            wardrobeBtn.style.display = 'none';
        } else if (currentSlot.isInWardrobe) {
            wardrobeBtn.style.display = 'block';
            wardrobeBtn.innerText = 'Transfer to Inventory';
            wardrobeBtn.setAttribute('data-action', 'retrieve');
        } else {
            wardrobeBtn.style.display = 'block';
            wardrobeBtn.innerText = 'Transfer to Cloud';
            wardrobeBtn.setAttribute('data-action', 'store');
        }
    }

    let posX = (typeof x === 'number' && x > 0) ? x : (window.innerWidth / 2);
    let posY = (typeof y === 'number' && y > 0) ? y : (window.innerHeight / 2);

    posX = Math.max(10, Math.min(posX, window.innerWidth - 200));
    posY = Math.max(10, Math.min(posY, window.innerHeight - 230));

    menu.style.left = posX + 'px';
    menu.style.top = posY + 'px';
    menu.style.display = 'flex';
    lastMenuOpenTime = Date.now();
}

function ctxToggleSlotWardrobe(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null || !currentWheelData) return;
    const wardrobeBtn = document.getElementById('ctx-slot-wardrobe-toggle');
    const action = wardrobeBtn ? wardrobeBtn.getAttribute('data-action') : 'store';
    if (action === 'retrieve') {
        if (typeof window.RetrieveSlotOutfit === 'function') {
            window.RetrieveSlotOutfit(JSON.stringify({
                wheelIndex: currentWheelData.wheelIndex,
                slotIndex: selectedSlotIndex
            }));
        }
        showSaveToast("Outfit Retrieved to Inventory!");
    } else {
        if (typeof window.StashSlotOutfit === 'function') {
            window.StashSlotOutfit(JSON.stringify({
                wheelIndex: currentWheelData.wheelIndex,
                slotIndex: selectedSlotIndex
            }));
        }
        showSaveToast("Outfit Stored in Cloud!");
    }
    hideAllMenus();
}

function showWheelContextMenu(x, y) {
    if (!isEditModeActive()) return;
    hideAllMenus();

    const menu = document.getElementById('wheel-context-menu');
    if (!menu) return;

    let posX = (typeof x === 'number' && x > 0) ? x : (window.innerWidth / 2);
    let posY = (typeof y === 'number' && y > 0) ? y : (window.innerHeight / 2);

    posX = Math.max(10, Math.min(posX, window.innerWidth - 200));
    posY = Math.max(10, Math.min(posY, window.innerHeight - 200));

    menu.style.left = posX + 'px';
    menu.style.top = posY + 'px';
    menu.style.display = 'flex';
    lastMenuOpenTime = Date.now();
}

function ctxStashAllOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.StashAllOutfits === 'function') {
        window.StashAllOutfits("");
    }
    showSaveToast("Inactive Outfits Stored in Cloud!");
    hideAllMenus();
}

function ctxRetrieveAllOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.RetrieveAllOutfits === 'function') {
        window.RetrieveAllOutfits("");
    }
    showSaveToast("All Outfits Retrieved!");
    hideAllMenus();
}

function handleCenterRightClick(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }
    if (!isEditModeActive()) return;
    showWheelContextMenu(e.clientX || 200, e.clientY || 200);
}

function openSettingsModal(e) {
    if (e) e.stopPropagation();
    hideAllMenus();
    hideAllModals();

    const modal = document.getElementById('settings-modal');
    const scaleInput = document.getElementById('scale-input');
    const offsetInputX = document.getElementById('offsetx-input');
    const offsetInputY = document.getElementById('offsety-input');

    const chkAutoStash = document.getElementById('chk-auto-stash');
    const chkMaster = document.getElementById('chk-auto-master');
    const chkCity = document.getElementById('chk-auto-city');
    const chkHome = document.getElementById('chk-auto-home');
    const chkCombat = document.getElementById('chk-auto-combat');
    const chkSwim = document.getElementById('chk-auto-swim');

    if (chkAutoStash) chkAutoStash.checked = currentWheelData.autoStashEnabled === true;
    if (chkMaster) chkMaster.checked = currentWheelData.autoSystemEnabled !== false;
    if (chkCity) chkCity.checked = currentWheelData.autoCityEnabled !== false;
    if (chkHome) chkHome.checked = currentWheelData.autoHomeEnabled !== false;
    if (chkCombat) chkCombat.checked = currentWheelData.autoCombatEnabled !== false;
    if (chkSwim) chkSwim.checked = currentWheelData.autoSwimEnabled !== false;

    tempScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
    tempOffsetX = (currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0;
    tempOffsetY = (currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0;
    tempEditKey = currentWheelData.editKey || 0x31;
    tempEditMod = currentWheelData.editModifierKey || 0;
    tempPlayKey = currentWheelData.playKey || 0x2F;
    tempPlayMod = currentWheelData.playModifierKey || 0;
    tempGamepadPlayKey = currentWheelData.gamepadPlayKey || 0x0008;
    tempGamepadPlayMod = currentWheelData.gamepadPlayModifier || 0x0100;

    if (scaleInput) scaleInput.value = Math.round(tempScale * 100) + '%';
    if (offsetInputX) offsetInputX.value = Math.round(tempOffsetX) + 'px';
    if (offsetInputY) offsetInputY.value = Math.round(tempOffsetY) + 'px';

    if (modal) modal.style.display = 'flex';
    setTyping(true);
}

function hideSettingsModal() {
    tempScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
    tempOffsetX = (currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0;
    tempOffsetY = (currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0;
    applyTransform();
    hideAllModals();
    setTyping(false);
}

function openKeyboardModal(e) {
    if (e) e.stopPropagation();
    hideAllModals();

    const kbModal = document.getElementById('keyboard-modal');
    const editBtn = document.getElementById('btn-edit-key');
    const playBtn = document.getElementById('btn-play-key');
    const editModText = document.getElementById('dd-edit-text');
    const playModText = document.getElementById('dd-play-text');

    if (editBtn) editBtn.innerText = '[' + getKeyName(tempEditKey) + ']';
    if (playBtn) playBtn.innerText = '[' + getKeyName(tempPlayKey) + ']';
    if (editModText) editModText.innerText = modNameMap[tempEditMod] || 'None';
    if (playModText) playModText.innerText = modNameMap[tempPlayMod] || 'None';

    if (kbModal) kbModal.style.display = 'flex';
    setTyping(true);
}

function hideKeyboardModal() {
    hideAllModals();
    const settingsModal = document.getElementById('settings-modal');
    if (settingsModal) settingsModal.style.display = 'flex';
}

function openGamepadModal(e) {
    if (e) e.stopPropagation();
    hideAllModals();

    const gpModal = document.getElementById('gamepad-modal');
    const gpPlayBtn = document.getElementById('btn-gp-play-key');
    const gpPlayModText = document.getElementById('dd-gp-play-text');

    if (gpPlayBtn) gpPlayBtn.innerText = '[' + (gamepadNames[tempGamepadPlayKey] ? gamepadNames[tempGamepadPlayKey] : ('0x' + tempGamepadPlayKey.toString(16).toUpperCase())) + ']';
    if (gpPlayModText) gpPlayModText.innerText = gpModNameMap[tempGamepadPlayMod] || 'None';

    if (gpModal) gpModal.style.display = 'flex';
    setTyping(true);
}

function hideGamepadModal() {
    hideAllModals();
    const settingsModal = document.getElementById('settings-modal');
    if (settingsModal) settingsModal.style.display = 'flex';
}

function saveKeyboardSettings() {
    currentWheelData.editKey = tempEditKey;
    currentWheelData.editModifierKey = tempEditMod;
    currentWheelData.playKey = tempPlayKey;
    currentWheelData.playModifierKey = tempPlayMod;
    saveSettings();
    hideKeyboardModal();
}

function saveGamepadSettings() {
    currentWheelData.gamepadPlayKey = tempGamepadPlayKey;
    currentWheelData.gamepadPlayModifier = tempGamepadPlayMod;
    saveSettings();
    hideGamepadModal();
}

function saveSettings() {
    currentWheelData.wheelScale = tempScale;
    currentWheelData.wheelOffsetX = tempOffsetX;
    currentWheelData.wheelOffsetY = tempOffsetY;
    currentWheelData.editKey = tempEditKey;
    currentWheelData.editModifierKey = tempEditMod;
    currentWheelData.playKey = tempPlayKey;
    currentWheelData.playModifierKey = tempPlayMod;
    currentWheelData.gamepadPlayKey = tempGamepadPlayKey;
    currentWheelData.gamepadPlayModifier = tempGamepadPlayMod;

    const chkAutoStash = document.getElementById('chk-auto-stash');
    const chkMaster = document.getElementById('chk-auto-master');
    const chkCity = document.getElementById('chk-auto-city');
    const chkHome = document.getElementById('chk-auto-home');
    const chkCombat = document.getElementById('chk-auto-combat');
    const chkSwim = document.getElementById('chk-auto-swim');

    const autoStashVal = chkAutoStash ? chkAutoStash.checked : false;
    const masterVal = chkMaster ? chkMaster.checked : true;
    const cityVal = chkCity ? chkCity.checked : true;
    const homeVal = chkHome ? chkHome.checked : true;
    const combatVal = chkCombat ? chkCombat.checked : true;
    const swimVal = chkSwim ? chkSwim.checked : true;

    currentWheelData.autoStashEnabled = autoStashVal;
    currentWheelData.autoSystemEnabled = masterVal;
    currentWheelData.autoCityEnabled = cityVal;
    currentWheelData.autoHomeEnabled = homeVal;
    currentWheelData.autoCombatEnabled = combatVal;
    currentWheelData.autoSwimEnabled = swimVal;

    if (typeof saveWheelPositionToStorage === 'function') saveWheelPositionToStorage();

    if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            editMode: isEditModeActive(),
            wheelScale: tempScale,
            wheelOffsetX: tempOffsetX,
            wheelOffsetY: tempOffsetY,
            editKey: tempEditKey,
            editModifierKey: tempEditMod,
            playKey: tempPlayKey,
            playModifierKey: tempPlayMod,
            gamepadPlayKey: tempGamepadPlayKey,
            gamepadPlayModifier: tempGamepadPlayMod,
            autoStashEnabled: autoStashVal,
            autoSystemEnabled: masterVal,
            autoCityEnabled: cityVal,
            autoHomeEnabled: homeVal,
            autoCombatEnabled: combatVal,
            autoSwimEnabled: swimVal
        }));
    }
    hideSettingsModal();
}

function openContextModal(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null) return;

    hideAllMenus();
    hideAllModals();

    const currentSlot = currentWheelData.slots.find(s => s.index === selectedSlotIndex);
    const currentTag = (currentSlot && currentSlot.contextTag) ? currentSlot.contextTag.toLowerCase() : "";
    const currentRating = (currentSlot && currentSlot.preferenceStars !== undefined) ? currentSlot.preferenceStars : 3;

    const chkCity = document.getElementById('chk-city');
    const chkHome = document.getElementById('chk-home');
    const chkCombat = document.getElementById('chk-combat');
    const chkSwim = document.getElementById('chk-swim');

    if (chkCity) chkCity.checked = currentTag.includes("city") || currentTag.includes("town");
    if (chkHome) chkHome.checked = currentTag.includes("home") || currentTag.includes("house");
    if (chkCombat) chkCombat.checked = currentTag.includes("combat") || currentTag.includes("fight");
    if (chkSwim) chkSwim.checked = currentTag.includes("swim") || currentTag.includes("water");

    setStarRating(currentRating);

    const modal = document.getElementById('context-modal');
    if (modal) modal.style.display = 'flex';
    setTyping(true);
}

function hideContextModal() {
    hideAllModals();
    setTyping(false);
}

function setStarRating(num) {
    tempStars = num;
    document.querySelectorAll('.star').forEach((s, idx) => {
        if (idx < num) s.classList.add('active');
        else s.classList.remove('active');
    });
}

function saveSlotContextAndRating() {
    if (selectedSlotIndex === null) return;

    const chkCity = document.getElementById('chk-city');
    const chkHome = document.getElementById('chk-home');
    const chkCombat = document.getElementById('chk-combat');
    const chkSwim = document.getElementById('chk-swim');

    let tags = [];
    if (chkCity && chkCity.checked) tags.push("City");
    if (chkHome && chkHome.checked) tags.push("Home");
    if (chkCombat && chkCombat.checked) tags.push("Combat");
    if (chkSwim && chkSwim.checked) tags.push("Swim");

    const contextString = tags.length > 0 ? tags.join(", ") : "";

    if (typeof window.SetSlotContext === 'function' && currentWheelData) {
        window.SetSlotContext(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            contextTag: contextString
        }));
    }

    if (typeof window.SetSlotRating === 'function' && currentWheelData) {
        window.SetSlotRating(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            ratingStars: tempStars
        }));
    }

    showSaveToast("Context & Rating Saved!");
    hideContextModal();
}

function showRenameModal(titleText, initialValue) {
    hideAllModals();
    const modal = document.getElementById('rename-modal');
    const title = document.getElementById('modal-title-text');
    const input = document.getElementById('rename-input');
    if (modal && title && input) {
        title.innerText = titleText;
        input.value = initialValue;
        modal.style.display = 'flex';
        setTimeout(() => {
            input.focus();
            input.select();
        }, 50);
    }
}

function hideRenameModal() {
    hideAllModals();
    setTyping(false);
}

function confirmRename() {
    const input = document.getElementById('rename-input');
    if (!input) return;

    const val = input.value.trim();
    if (!val) {
        hideRenameModal();
        return;
    }

    if (renameTargetMode === 'slot' && selectedSlotIndex !== null) {
        if (typeof window.RenameSlot === 'function' && currentWheelData) {
            window.RenameSlot(JSON.stringify({
                wheelIndex: currentWheelData.wheelIndex,
                slotIndex: selectedSlotIndex,
                newName: val
            }));
        }
    } else if (renameTargetMode === 'wheel') {
        if (typeof window.RenameWheel === 'function' && currentWheelData) {
            window.RenameWheel(JSON.stringify({
                wheelIndex: currentWheelData.wheelIndex,
                newName: val
            }));
        }
    }
    hideRenameModal();
}

function handleRenameKey(e) {
    if (e.key === 'Enter') confirmRename();
    else if (e.key === 'Escape') hideRenameModal();
}

function ctxSaveOutfit(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex !== null) saveOutfit(selectedSlotIndex);
    hideAllMenus();
}

function ctxRenameSlot(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null) return;
    renameTargetMode = 'slot';
    const currentSlot = currentWheelData.slots.find(s => s.index === selectedSlotIndex);
    const currentName = (currentSlot && currentSlot.hasOutfit) ? currentSlot.name : ('Outfit ' + (selectedSlotIndex + 1));
    showRenameModal("Rename Preset", currentName);
    hideAllMenus();
}

function ctxClearSlot(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex !== null && typeof window.ClearSlot === 'function' && currentWheelData) {
        window.ClearSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex
        }));
    }
    selectedSlotIndex = 0;
    hideAllMenus();
}

function ctxDeleteSlot(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex !== null && typeof window.DeleteSlot === 'function' && currentWheelData) {
        window.DeleteSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex
        }));
    }
    selectedSlotIndex = 0;
    hideAllMenus();
}

function ctxRenameWheel(e) {
    if (e) e.stopPropagation();
    renameTargetMode = 'wheel';
    const currentName = currentWheelData.wheelName || ('Wheel ' + (currentWheelData.wheelIndex + 1));
    showRenameModal("Rename Wheel", currentName);
    hideAllMenus();
}

function ctxDeleteWheel(e) {
    if (e) e.stopPropagation();
    if (typeof window.DeleteWheel === 'function' && currentWheelData) {
        window.DeleteWheel(JSON.stringify({}));
    }
    hideAllMenus();
}

function stepScale(delta) {
    tempScale = Math.max(0.1, tempScale + delta);
    currentWheelData.wheelScale = tempScale;
    const scaleInput = document.getElementById('scale-input');
    if (scaleInput) scaleInput.value = Math.round(tempScale * 100) + '%';
    applyTransform();
}

function stepOffsetX(delta) {
    tempOffsetX += delta;
    currentWheelData.wheelOffsetX = tempOffsetX;
    const offsetInput = document.getElementById('offsetx-input');
    if (offsetInput) offsetInput.value = Math.round(tempOffsetX) + 'px';
    applyTransform();
}

function stepOffsetY(delta) {
    tempOffsetY += delta;
    currentWheelData.wheelOffsetY = tempOffsetY;
    const offsetInput = document.getElementById('offsety-input');
    if (offsetInput) offsetInput.value = Math.round(tempOffsetY) + 'px';
    applyTransform();
}

function onScaleInputChange(val) {
    let clean = String(val).replace('%', '').trim();
    let num = parseFloat(clean);
    if (!isNaN(num)) {
        if (num > 10) num /= 100.0;
        tempScale = Math.max(0.1, num);
        currentWheelData.wheelScale = tempScale;
    }
    const scaleInput = document.getElementById('scale-input');
    if (scaleInput) scaleInput.value = Math.round(tempScale * 100) + '%';
    applyTransform();
}

function onScaleInputBlur() {
    setTyping(false);
    const scaleInput = document.getElementById('scale-input');
    if (scaleInput) onScaleInputChange(scaleInput.value);
}

function onOffsetXInputChange(val) {
    let clean = String(val).replace('px', '').trim();
    let num = parseFloat(clean);
    if (!isNaN(num)) {
        tempOffsetX = num;
        currentWheelData.wheelOffsetX = tempOffsetX;
    }
    const offsetInput = document.getElementById('offsetx-input');
    if (offsetInput) offsetInput.value = Math.round(tempOffsetX) + 'px';
    applyTransform();
}

function onOffsetXInputBlur() {
    setTyping(false);
    const offsetInput = document.getElementById('offsetx-input');
    if (offsetInput) onOffsetXInputChange(offsetInput.value);
}

function onOffsetYInputChange(val) {
    let clean = String(val).replace('px', '').trim();
    let num = parseFloat(clean);
    if (!isNaN(num)) {
        tempOffsetY = num;
        currentWheelData.wheelOffsetY = tempOffsetY;
    }
    const offsetInput = document.getElementById('offsety-input');
    if (offsetInput) offsetInput.value = Math.round(tempOffsetY) + 'px';
    applyTransform();
}

function onOffsetYInputBlur() {
    setTyping(false);
    const offsetInput = document.getElementById('offsety-input');
    if (offsetInput) onOffsetYInputChange(offsetInput.value);
}

function handleScaleScroll(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
        stepScale(e.deltaY < 0 ? 0.05 : -0.05);
    }
}

function handleOffsetScroll(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
        stepOffsetX(e.deltaY < 0 ? 10 : -10);
    }
}

function handleOffsetYScroll(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
        stepOffsetY(e.deltaY < 0 ? 10 : -10);
    }
}

function applyTransform() {
    const container = document.getElementById('wheel-container');
    if (container) {
        const isEdit = isEditModeActive();
        const baseScale = isEdit ? 0.55 : 1.0;
        const curScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : (tempScale || 1.0);
        const curOffsetX = isEdit ? 0.0 : ((currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : (tempOffsetX || 0.0));
        const curOffsetY = isEdit ? 0.0 : ((currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : (tempOffsetY || 0.0));
        container.style.transform = 'translateX(' + curOffsetX + 'px) translateY(' + curOffsetY + 'px) scale(' + (baseScale * curScale) + ')';
    }
}

function toggleDropdown(type, e) {
    if (e) e.stopPropagation();
    const map = {
        'edit': document.getElementById('dd-edit-list'),
        'play': document.getElementById('dd-play-list'),
        'gp-play': document.getElementById('dd-gp-play-list')
    };
    for (let key in map) {
        if (map[key]) {
            if (key === type) map[key].style.display = map[key].style.display === 'none' ? 'flex' : 'none';
            else map[key].style.display = 'none';
        }
    }
}

function selectMod(type, val, text, e) {
    if (e) e.stopPropagation();
    if (type === 'edit') {
        tempEditMod = val;
        document.getElementById('dd-edit-text').innerText = text;
    } else if (type === 'play') {
        tempPlayMod = val;
        document.getElementById('dd-play-text').innerText = text;
    } else if (type === 'gp-play') {
        tempGamepadPlayMod = val;
        document.getElementById('dd-gp-play-text').innerText = text;
    }
    closeAllDropdowns();
}

function closeAllDropdowns() {
    ['dd-edit-list', 'dd-play-list', 'dd-gp-play-list'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
}

function renderDrawerDetails() {
    const badge = document.getElementById('drawer-slot-badge');
    const nameEl = document.getElementById('drawer-outfit-name');
    const statusContainer = document.getElementById('drawer-slot-status');
    const btnFull = document.getElementById('btn-type-full');
    const btnAddon = document.getElementById('btn-type-addon');

    if (!badge || !nameEl || !currentWheelData || !currentWheelData.slots) return;

    const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
    if (!slot) return;

    badge.innerText = 'SLOT ' + (selectedSlotIndex + 1);
    nameEl.innerText = slot.hasOutfit ? slot.name : 'Empty Slot';

    if (statusContainer) {
        if (slot.isAddon) {
            statusContainer.style.display = 'inline-block';
            statusContainer.className = 'status-chip chip-addon';
            statusContainer.innerText = 'ADDON';
        } else if (slot.isWorn || slot.isEquipped) {
            statusContainer.style.display = 'inline-block';
            statusContainer.className = 'status-chip chip-equipped';
            statusContainer.innerText = 'EQUIPPED';
        } else {
            statusContainer.style.display = 'none';
        }
    }

    if (btnFull && btnAddon) {
        if (slot.isAddon) {
            btnFull.classList.remove('active');
            btnAddon.classList.add('active');
        } else {
            btnFull.classList.add('active');
            btnAddon.classList.remove('active');
        }
    }

    const currentStars = (slot && slot.preferenceStars !== undefined) ? slot.preferenceStars : 3;
    const stars = document.querySelectorAll('#stars-widget .star-icon');
    stars.forEach((s, idx) => {
        if (idx < currentStars) s.classList.add('active');
        else s.classList.remove('active');
    });

    const currentTag = (slot && slot.contextTag) ? slot.contextTag.toLowerCase() : '';
    ['combat', 'city', 'home', 'swim'].forEach(t => {
        const chip = document.getElementById('ctx-chip-' + t);
        if (chip) {
            if (currentTag.includes(t)) chip.classList.add('active');
            else chip.classList.remove('active');
        }
    });

    renderDrawerItemsList(slot);
}

function renderDrawerItemsList(slot) {
    const list = document.getElementById('drawer-items-list');
    if (!list) return;
    list.innerHTML = '';

    const items = slot.items || slot.itemIDs || [];
    if (!slot.hasOutfit || items.length === 0) {
        list.innerHTML = '<div style="font-size:10px; color:#64748b; text-align:center; padding:16px;">No items saved in this slot. Click \'Save Current Gear\' to assign loadout.</div>';
        return;
    }

    items.forEach(itemEntry => {
        let itemObj = null;
        if (typeof itemEntry === 'object' && itemEntry.name) {
            itemObj = itemEntry;
        } else {
            const formIdStr = (typeof itemEntry === 'number') ? ('0x' + itemEntry.toString(16).toUpperCase()) : String(itemEntry);
            itemObj = (currentWheelData.inventory || []).find(x => x.id === formIdStr || x.rawID === itemEntry) || {
                name: 'Apparel Item (' + formIdStr + ')',
                type: 'Armor',
                icon: '🛡️'
            };
        }
        const row = document.createElement('div');
        row.className = 'drawer-item-row';
        row.innerHTML = '<div class="drawer-item-info"><span>' + (itemObj.icon || '🛡️') + '</span><span>' + itemObj.name + '</span></div><span class="drawer-item-slot-tag">' + (itemObj.type || 'Apparel') + '</span>';
        list.appendChild(row);
    });
}

function toggleSlotAddonType(isAddon) {
    if (typeof window.SetSlotType === 'function' && currentWheelData) {
        window.SetSlotType(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            isAddon: isAddon
        }));
    }
}

function setSlotRatingDirect(num) {
    if (typeof window.SetSlotRating === 'function' && currentWheelData) {
        window.SetSlotRating(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            ratingStars: num
        }));
    }
}

function toggleContextTagDirect(ctxKey) {
    const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
    if (!slot) return;

    let currentTags = (slot.contextTag || '').split(',').map(x => x.trim()).filter(x => x.length > 0);
    if (currentTags.includes(ctxKey)) {
        currentTags = currentTags.filter(x => x !== ctxKey);
    } else {
        currentTags.push(ctxKey);
    }

    const newTagStr = currentTags.join(', ');
    if (typeof window.SetSlotContext === 'function' && currentWheelData) {
        window.SetSlotContext(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            contextTag: newTagStr
        }));
    }
}

function saveCurrentEquippedToSlot() {
    saveOutfit(selectedSlotIndex);
}

function clearSelectedSlot() {
    ctxClearSlot();
}