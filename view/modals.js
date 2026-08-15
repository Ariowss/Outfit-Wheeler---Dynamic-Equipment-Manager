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
            wardrobeBtn.innerText = 'Retrieve to Inventory';
            wardrobeBtn.setAttribute('data-action', 'retrieve');
        } else {
            wardrobeBtn.style.display = 'block';
            wardrobeBtn.innerText = 'Store in Cloud';
            wardrobeBtn.setAttribute('data-action', 'store');
        }
    }

    let posX = (typeof x === 'number' && x > 0) ? x : (window.innerWidth / 2);
    let posY = (typeof y === 'number' && y > 0) ? y : (window.innerHeight / 2);

    if (posX + 190 > window.innerWidth) posX = window.innerWidth - 200;
    if (posY + 220 > window.innerHeight) posY = window.innerHeight - 230;

    menu.style.left = Math.max(10, posX) + 'px';
    menu.style.top = Math.max(10, posY) + 'px';
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

    menu.style.left = Math.min(posX, window.innerWidth - 180) + 'px';
    menu.style.top = Math.min(posY, window.innerHeight - 180) + 'px';
    menu.style.display = 'flex';
    lastMenuOpenTime = Date.now();
}

function ctxStashWheelOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.StashWheelOutfits === 'function' && currentWheelData) {
        window.StashWheelOutfits(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex
        }));
    }
    showSaveToast("Wheel Outfits Stored in Cloud!");
    hideAllMenus();
}

function ctxRetrieveWheelOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.RetrieveWheelOutfits === 'function' && currentWheelData) {
        window.RetrieveWheelOutfits(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex
        }));
    }
    showSaveToast("Wheel Outfits Retrieved to Inventory!");
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
    const offsetInput = document.getElementById('offsetx-input');

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

    tempScale = currentWheelData.wheelScale || 1.0;
    tempOffsetX = currentWheelData.wheelOffsetX || 0.0;
    tempEditKey = currentWheelData.editKey || 0x31;
    tempEditMod = currentWheelData.editModifierKey || 0;
    tempPlayKey = currentWheelData.playKey || 0x2F;
    tempPlayMod = currentWheelData.playModifierKey || 0;
    tempGamepadPlayKey = currentWheelData.gamepadPlayKey || 0x0008;
    tempGamepadPlayMod = currentWheelData.gamepadPlayModifier || 0x0100;

    if (scaleInput) scaleInput.value = Math.round(tempScale * 100) + '%';
    if (offsetInput) offsetInput.value = Math.round(tempOffsetX) + 'px';

    if (modal) modal.style.display = 'flex';
    setTyping(true);
}

function hideSettingsModal() {
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

    if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            wheelScale: tempScale,
            wheelOffsetX: tempOffsetX,
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

function retrieveAllOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.RetrieveAllOutfits === 'function') {
        window.RetrieveAllOutfits("");
    }
    showSaveToast("All Outfits Retrieved!");
}

function stashAllOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.StashAllOutfits === 'function') {
        window.StashAllOutfits("");
    }
    showSaveToast("Inactive Outfits Stored in Cloud!");
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
    showRenameModal("Rename Slot", currentName);
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
    selectedSlotIndex = null;
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
    selectedSlotIndex = null;
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
    const scaleInput = document.getElementById('scale-input');
    if (scaleInput) scaleInput.value = Math.round(tempScale * 100) + '%';
    applyTransform();
}

function stepOffsetX(delta) {
    tempOffsetX += delta;
    const offsetInput = document.getElementById('offsetx-input');
    if (offsetInput) offsetInput.value = Math.round(tempOffsetX) + 'px';
    applyTransform();
}

function onScaleInputChange(val) {
    let clean = String(val).replace('%', '').trim();
    let num = parseFloat(clean);
    if (!isNaN(num)) {
        if (num > 10) num /= 100.0;
        tempScale = Math.max(0.1, num);
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
    if (!isNaN(num)) tempOffsetX = num;
    const offsetInput = document.getElementById('offsetx-input');
    if (offsetInput) offsetInput.value = Math.round(tempOffsetX) + 'px';
    applyTransform();
}

function onOffsetXInputBlur() {
    setTyping(false);
    const offsetInput = document.getElementById('offsetx-input');
    if (offsetInput) onOffsetXInputChange(offsetInput.value);
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

function applyTransform() {
    const container = document.getElementById('wheel-container');
    if (container) {
        container.style.transform = 'translateX(' + tempOffsetX + 'px) scale(' + tempScale + ')';
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