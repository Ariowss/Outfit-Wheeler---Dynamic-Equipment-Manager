var renameTargetMode = 'slot';
var tempStars = 3;
var lastMenuOpenTime = 0;
var toastTimeout = null;

var modNameMap = { 0: 'None', 42: 'Shift', 29: 'Ctrl', 56: 'Alt' };
var gpModNameMap = { 0: 'None', 256: 'LB', 512: 'RB', 9: 'LT', 10: 'RT' };

const ALL_CONDITIONS_LIST = [
    { id: "combat", label: "Combat", cat: "action" },
    { id: "in_water", label: "In Water", cat: "action" },
    { id: "swim", label: "Swimming", cat: "action" },
    { id: "sleep", label: "Sleeping", cat: "action" },
    { id: "mount", label: "Mounting", cat: "action" },

    { id: "world", label: "World (Adventure)", cat: "generic" },
    { id: "town", label: "Town", cat: "generic" },
    { id: "city", label: "City", cat: "generic" },
    { id: "day", label: "Day", cat: "modifier" },
    { id: "night", label: "Night", cat: "modifier" },
    { id: "rain", label: "Rain", cat: "modifier" },
    { id: "snow", label: "Snow", cat: "modifier" },
    { id: "interior", label: "Interior", cat: "modifier" },

    { id: "dungeon", label: "Dungeon", cat: "unique" },
    { id: "home", label: "Player Home", cat: "unique" },
    { id: "inn", label: "Inn & Tavern", cat: "unique" },
    { id: "store", label: "Store & Market", cat: "unique" },
    { id: "guild", label: "Guild Hall", cat: "unique" },
    { id: "castle", label: "Castle & Palace", cat: "unique" },
    { id: "temple", label: "Temple & Shrine", cat: "unique" },
    { id: "farm", label: "Farm & Mill", cat: "unique" },
    { id: "jail", label: "Jail & Prison", cat: "unique" },
    { id: "military", label: "Military Camp", cat: "unique" }
];

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
    ['rename-modal', 'context-modal'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    const panelSettings = document.getElementById('panel-settings');
    if (panelSettings) panelSettings.style.display = 'none';

    listeningKeyType = null;
    listeningGamepadType = null;
    closeAllDropdowns();
    setTyping(false);
}

function hideAllMenus(e) {
    if (e && typeof e.stopPropagation === 'function') e.stopPropagation();
    ['slot-context-menu', 'wheel-context-menu', 'item-context-menu', 'transfer-all-context-menu'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    closeAllDropdowns();
}

function showSlotContextMenu(x, y, slotIndex) {
    if (!isEditModeActive()) return;

    const menu = document.getElementById('slot-context-menu');
    if (!menu) return;

    if (menu.style.display === 'flex' && selectedSlotIndex === slotIndex) {
        hideAllMenus();
        return;
    }

    selectedSlotIndex = slotIndex;
    hideAllMenus();

    const title = document.getElementById('slot-ctx-title');
    if (title) {
        title.innerText = 'Slot ' + (slotIndex + 1) + ' Options';
    }

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
            wardrobeBtn.innerText = 'Store in Wardrobe';
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
        showSaveToast("Outfit Stored in Wardrobe!");
    }
    hideAllMenus();
}

function showWheelContextMenu(x, y) {
    if (!isEditModeActive()) return;

    const menu = document.getElementById('wheel-context-menu');
    if (!menu) return;

    if (menu.style.display === 'flex') {
        hideAllMenus();
        return;
    }

    hideAllMenus();

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
    showSaveToast("Inactive Outfits Stored in Wardrobe!");
    hideAllMenus();
}

function ctxRetrieveAllOutfits(e) {
    if (e) e.stopPropagation();
    if (typeof window.RetrieveAllOutfits === 'function') {
        window.RetrieveAllOutfits("");
    }
    showSaveToast("All Outfits Retrieved from Wardrobe!");
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

    const panel = document.getElementById('panel-settings');
    if (!panel) return;

    const isVisible = panel.style.display !== 'none';
    if (isVisible) {
        hideSettingsModal();
        return;
    }

    const chkAutoClose = document.getElementById('chk-auto-close-equip');
    if (chkAutoClose) chkAutoClose.checked = currentWheelData.autoCloseOnEquip !== false;

    tempEditKey = currentWheelData.editKey || 0x31;
    tempEditMod = currentWheelData.editModifierKey || 0;
    tempPlayKey = currentWheelData.playKey || 0x2F;
    tempPlayMod = currentWheelData.playModifierKey || 0;
    tempGamepadPlayKey = currentWheelData.gamepadPlayKey || 0x0008;
    tempGamepadPlayMod = currentWheelData.gamepadPlayModifier || 0x0100;

    const editBtn = document.getElementById('btn-edit-key');
    const playBtn = document.getElementById('btn-play-key');
    const gpPlayBtn = document.getElementById('btn-gp-play-key');
    const editModText = document.getElementById('dd-edit-text');
    const playModText = document.getElementById('dd-play-text');
    const gpPlayModText = document.getElementById('dd-gp-play-text');

    if (editBtn) editBtn.innerText = '[' + getKeyName(tempEditKey) + ']';
    if (playBtn) playBtn.innerText = '[' + getKeyName(tempPlayKey) + ']';
    if (gpPlayBtn) gpPlayBtn.innerText = '[' + (gamepadNames[tempGamepadPlayKey] ? gamepadNames[tempGamepadPlayKey] : ('0x' + tempGamepadPlayKey.toString(16).toUpperCase())) + ']';

    if (editModText) editModText.innerText = modNameMap[tempEditMod] || 'None';
    if (playModText) playModText.innerText = modNameMap[tempPlayMod] || 'None';
    if (gpPlayModText) gpPlayModText.innerText = gpModNameMap[tempGamepadPlayMod] || 'None';

    panel.style.display = 'flex';
    setTyping(true);
}

function hideSettingsModal() {
    const panel = document.getElementById('panel-settings');
    if (panel) panel.style.display = 'none';

    listeningKeyType = null;
    listeningGamepadType = null;
    closeAllDropdowns();
    setTyping(false);
}

function saveSettings() {
    const chkAutoClose = document.getElementById('chk-auto-close-equip');
    const autoCloseVal = chkAutoClose ? chkAutoClose.checked : true;

    currentWheelData.autoCloseOnEquip = autoCloseVal;
    currentWheelData.editKey = tempEditKey;
    currentWheelData.editModifierKey = tempEditMod;
    currentWheelData.playKey = tempPlayKey;
    currentWheelData.playModifierKey = tempPlayMod;
    currentWheelData.gamepadPlayKey = tempGamepadPlayKey;
    currentWheelData.gamepadPlayModifier = tempGamepadPlayMod;

    if (typeof saveWheelPositionToStorage === 'function') saveWheelPositionToStorage();
    if (typeof savePanelsLayout === 'function') savePanelsLayout();

    if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            editMode: isEditModeActive(),
            wheelScale: currentWheelData.wheelScale || 1.0,
            wheelOffsetX: currentWheelData.wheelOffsetX || 0.0,
            wheelOffsetY: currentWheelData.wheelOffsetY || 0.0,
            editKey: tempEditKey,
            editModifierKey: tempEditMod,
            playKey: tempPlayKey,
            playModifierKey: tempPlayMod,
            gamepadPlayKey: tempGamepadPlayKey,
            gamepadPlayModifier: tempGamepadPlayMod,
            autoCloseOnEquip: autoCloseVal,
            autoStashEnabled: currentWheelData.autoStashEnabled === true,
            autoSystemEnabled: currentWheelData.autoSystemEnabled !== false,
            autoCityEnabled: currentWheelData.autoCityEnabled !== false,
            autoHomeEnabled: currentWheelData.autoHomeEnabled !== false,
            autoCombatEnabled: currentWheelData.autoCombatEnabled !== false,
            autoSwimEnabled: currentWheelData.autoSwimEnabled !== false
        }));
    }
    hideSettingsModal();
    showSaveToast("Settings Saved!");
}

function openContextModal(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null) return;

    hideAllMenus();
    hideAllModals();

    const currentSlot = currentWheelData.slots.find(s => s.index === selectedSlotIndex);
    const currentTag = (currentSlot && currentSlot.contextTag) ? currentSlot.contextTag.toLowerCase() : "";
    const currentRating = (currentSlot && currentSlot.preferenceStars !== undefined) ? currentSlot.preferenceStars : 3;

    const container = document.getElementById('modal-conditions-checkboxes-container');
    if (container) {
        container.innerHTML = '';
        ALL_CONDITIONS_LIST.forEach(cond => {
            const isChecked = currentTag.includes(cond.id.toLowerCase());
            const row = document.createElement('div');
            row.className = 'checkbox-row';
            row.innerHTML = '<input type="checkbox" id="chk-' + cond.id + '" ' + (isChecked ? 'checked' : '') + '><label for="chk-' + cond.id + '">' + cond.label + '</label>';
            container.appendChild(row);
        });
    }

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

    let tags = [];
    ALL_CONDITIONS_LIST.forEach(cond => {
        const chk = document.getElementById('chk-' + cond.id);
        if (chk && chk.checked) {
            tags.push(cond.id);
        }
    });

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

    showSaveToast("Contexts & Rating Saved!");
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
        setTyping(true);
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
    const currentName = (currentSlot && currentSlot.name) ? currentSlot.name : ('Outfit ' + (selectedSlotIndex + 1));
    showRenameModal("Rename Preset", currentName);
    hideAllMenus();
}

function ctxMoveSlot(delta, e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null || !currentWheelData || !currentWheelData.slots) return;
    const total = currentWheelData.slots.length;
    if (total <= 1) return;

    const fromIdx = selectedSlotIndex;
    const toIdx = fromIdx + delta;
    if (toIdx < 0 || toIdx >= total) return;

    if (typeof window.MoveSlot === 'function') {
        window.MoveSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            fromIndex: fromIdx,
            toIndex: toIdx,
            editMode: isEditModeActive()
        }));
    }

    const temp = currentWheelData.slots[fromIdx];
    currentWheelData.slots[fromIdx] = currentWheelData.slots[toIdx];
    currentWheelData.slots[toIdx] = temp;
    currentWheelData.slots.forEach((s, i) => s.index = i);

    selectedSlotIndex = toIdx;
    renderSlotsRail();
    renderDrawerDetails();
    if (typeof updateWheelUI === 'function') updateWheelUI();
    showSaveToast("Slot Reordered!");
    hideAllMenus();
}

function ctxMoveWheel(delta, e) {
    if (e) e.stopPropagation();
    if (!currentWheelData || currentWheelData.totalWheels <= 1) return;

    const fromIdx = currentWheelData.wheelIndex || 0;
    const toIdx = fromIdx + delta;
    if (toIdx < 0 || toIdx >= currentWheelData.totalWheels) return;

    if (typeof window.MoveWheel === 'function') {
        window.MoveWheel(JSON.stringify({
            fromIndex: fromIdx,
            toIndex: toIdx,
            editMode: isEditModeActive()
        }));
    }
    hideAllMenus();
}

function ctxClearSlot(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }

    if (selectedSlotIndex === null || selectedSlotIndex === undefined) return;

    if (typeof window.ClearSlot === 'function' && currentWheelData) {
        window.ClearSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: selectedSlotIndex
        }));
    }

    if (currentWheelData && currentWheelData.slots) {
        const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
        if (slot) {
            slot.hasOutfit = false;
            slot.name = 'Outfit ' + (selectedSlotIndex + 1);
            slot.items = [];
            slot.itemIDs = [];
            slot.count = 0;
            slot.isEquipped = false;
            slot.isWorn = false;
            slot.contextTag = '';
        }
    }

    renderSlotsRail();
    renderDrawerDetails();
    if (typeof updateWheelUI === 'function') updateWheelUI();
    showSaveToast("Slot Cleared!");
    hideAllMenus();
}

function ctxDeleteSlot(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }
    if (selectedSlotIndex === null || selectedSlotIndex === undefined || !currentWheelData || !currentWheelData.slots) return;

    const targetIdx = selectedSlotIndex;

    if (typeof window.DeleteSlot === 'function') {
        window.DeleteSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: targetIdx
        }));
    }

    if (currentWheelData.slots.length > 0) {
        currentWheelData.slots.splice(targetIdx, 1);
        currentWheelData.slots.forEach((s, i) => s.index = i);
        selectedSlotIndex = currentWheelData.slots.length > 0 ? Math.max(0, Math.min(targetIdx, currentWheelData.slots.length - 1)) : 0;
    }

    renderSlotsRail();
    renderDrawerDetails();
    if (typeof updateWheelUI === 'function') updateWheelUI();
    showSaveToast("Slot Deleted!");
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