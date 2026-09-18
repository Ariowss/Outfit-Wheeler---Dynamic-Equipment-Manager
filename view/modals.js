if (typeof window.safeGetStorage === 'undefined') {
    window.safeGetStorage = function(key, defaultVal) {
        try {
            const val = localStorage.getItem(key);
            return (val !== null && val !== undefined) ? val : defaultVal;
        } catch (err) {
            return defaultVal;
        }
    };
}
if (typeof window.safeSetStorage === 'undefined') {
    window.safeSetStorage = function(key, val) {
        try {
            localStorage.setItem(key, String(val));
        } catch (err) {}
    };
}
var safeGetStorage = window.safeGetStorage;
var safeSetStorage = window.safeSetStorage;

var renameTargetMode = 'slot';
var tempStars = 3;
var lastMenuOpenTime = 0;
var toastTimeout = null;

var modNameMap = {
    0: 'None',
    '0': 'None',
    42: 'Shift',
    '42': 'Shift',
    54: 'Shift',
    '54': 'Shift',
    0x2A: 'Shift',
    0x36: 'Shift',
    29: 'Ctrl',
    '29': 'Ctrl',
    157: 'Ctrl',
    '157': 'Ctrl',
    0x1D: 'Ctrl',
    0x9D: 'Ctrl',
    56: 'Alt',
    '56': 'Alt',
    184: 'Alt',
    '184': 'Alt',
    0x38: 'Alt',
    0xB8: 'Alt'
};
var gpModNameMap = {
    0: 'None',
    '0': 'None',
    256: 'LB',
    '256': 'LB',
    0x0100: 'LB',
    512: 'RB',
    '512': 'RB',
    0x0200: 'RB',
    9: 'LT',
    '9': 'LT',
    0x0009: 'LT',
    10: 'RT',
    '10': 'RT',
    0x000A: 'RT'
};

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

function hideAllModals(keepSettingsOpen) {
    ['rename-modal', 'context-modal'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    if (!keepSettingsOpen) {
        const panelSettings = document.getElementById('panel-settings');
        if (panelSettings) panelSettings.style.display = 'none';
        listeningKeyType = null;
        listeningGamepadType = null;
        setTyping(false);
    }
    closeAllDropdowns();
}

function showSlotContextMenu(x, y, slotIndex) {
    const menu = document.getElementById('slot-context-menu');
    if (!menu) return;

    const now = Date.now();
    if (menu.style.display === 'flex' && selectedSlotIndex === slotIndex) {
        if (now - lastMenuOpenTime < 250) return;
        hideAllMenus();
        return;
    }

    selectedSlotIndex = slotIndex;
    hideAllMenus();

    const title = document.getElementById('slot-ctx-title');
    if (title) {
        title.innerText = 'Slot ' + (slotIndex + 1) + ' Options';
    }

    const currentSlot = (currentWheelData.slots || []).find(s => (s.index !== undefined ? s.index : 0) === slotIndex) || (currentWheelData.slots || [])[slotIndex];
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

    if (typeof positionContextMenu === 'function') {
        positionContextMenu(menu, x, y);
    } else if (typeof window.positionContextMenu === 'function') {
        window.positionContextMenu(menu, x, y);
    } else {
        const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
        menu.style.visibility = 'hidden';
        menu.style.display = 'flex';
        const menuW = menu.offsetWidth || 180;
        const menuH = menu.offsetHeight || 260;
        const rawVw = window.innerWidth || (document.documentElement ? document.documentElement.clientWidth : 1920) || 1920;
        const rawVh = window.innerHeight || (document.documentElement ? document.documentElement.clientHeight : 1080) || 1080;
        const vw = rawVw / scale;
        const vh = rawVh / scale;
        let posX = (typeof x === 'number' && !isNaN(x)) ? (x / scale) : (vw / 2);
        let posY = (typeof y === 'number' && !isNaN(y)) ? (y / scale) : (vh / 2);
        if (posX + menuW > vw - 15) posX = Math.max(15, posX - menuW);
        if (posY + menuH > vh - 15) {
            if (posY - menuH >= 15) posY = posY - menuH;
            else posY = Math.max(15, vh - menuH - 15);
        }
        posX = Math.max(15, Math.min(posX, Math.max(15, vw - menuW - 15)));
        posY = Math.max(15, Math.min(posY, Math.max(15, vh - menuH - 15)));
        menu.style.left = Math.round(posX) + 'px';
        menu.style.top = Math.round(posY) + 'px';
        menu.style.visibility = 'visible';
    }
    lastMenuOpenTime = now;
}

function ctxToggleSlotWardrobe(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null || !currentWheelData) return;
    const wardrobeBtn = document.getElementById('ctx-slot-wardrobe-toggle');
    const action = wardrobeBtn ? wardrobeBtn.getAttribute('data-action') : 'store';
    if (action === 'retrieve') {
        if (typeof window.RetrieveSlotOutfit === 'function') {
            window.RetrieveSlotOutfit(JSON.stringify({
                wheelIndex: currentWheelData.wheelIndex || 0,
                slotIndex: selectedSlotIndex
            }));
        }
        showSaveToast("Outfit Retrieved to Inventory!");
    } else {
        if (typeof window.StashSlotOutfit === 'function') {
            window.StashSlotOutfit(JSON.stringify({
                wheelIndex: currentWheelData.wheelIndex || 0,
                slotIndex: selectedSlotIndex
            }));
        }
        showSaveToast("Outfit Stored in Wardrobe!");
    }
    hideAllMenus();
}

function showWheelContextMenu(x, y) {
    const menu = document.getElementById('wheel-context-menu');
    if (!menu) return;

    const now = Date.now();
    if (menu.style.display === 'flex') {
        if (now - lastMenuOpenTime < 250) return;
        hideAllMenus();
        return;
    }

    hideAllMenus();

    if (typeof positionContextMenu === 'function') {
        positionContextMenu(menu, x, y);
    } else if (typeof window.positionContextMenu === 'function') {
        window.positionContextMenu(menu, x, y);
    } else {
        const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
        menu.style.visibility = 'hidden';
        menu.style.display = 'flex';
        const menuW = menu.offsetWidth || 180;
        const menuH = menu.offsetHeight || 200;
        const rawVw = window.innerWidth || (document.documentElement ? document.documentElement.clientWidth : 1920) || 1920;
        const rawVh = window.innerHeight || (document.documentElement ? document.documentElement.clientHeight : 1080) || 1080;
        const vw = rawVw / scale;
        const vh = rawVh / scale;
        let posX = (typeof x === 'number' && !isNaN(x)) ? (x / scale) : (vw / 2);
        let posY = (typeof y === 'number' && !isNaN(y)) ? (y / scale) : (vh / 2);
        if (posX + menuW > vw - 15) posX = Math.max(15, posX - menuW);
        if (posY + menuH > vh - 15) {
            if (posY - menuH >= 15) posY = posY - menuH;
            else posY = Math.max(15, vh - menuH - 15);
        }
        posX = Math.max(15, Math.min(posX, Math.max(15, vw - menuW - 15)));
        posY = Math.max(15, Math.min(posY, Math.max(15, vh - menuH - 15)));
        menu.style.left = Math.round(posX) + 'px';
        menu.style.top = Math.round(posY) + 'px';
        menu.style.visibility = 'visible';
    }
    lastMenuOpenTime = now;
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
    if (typeof isDraggingWheelCenter !== 'undefined' && isDraggingWheelCenter) return;
    if (typeof wheelDragMoved !== 'undefined' && wheelDragMoved) return;
    const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (window.innerWidth / 2);
    const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (window.innerHeight / 2);
    showWheelContextMenu(clickX, clickY);
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

    panel.style.position = 'fixed';
    panel.style.left = '50%';
    panel.style.top = '50%';
    panel.style.transform = 'translate(-50%, -50%)';

    const chkAutoSystem = document.getElementById('chk-auto-system');
    if (chkAutoSystem) chkAutoSystem.checked = (currentWheelData.autoSystemEnabled === true);

    const chkAutoClose = document.getElementById('chk-auto-close-equip');
    if (chkAutoClose) chkAutoClose.checked = (currentWheelData.autoCloseOnEquip !== false);

    const chkAutoStash = document.getElementById('chk-auto-stash');
    if (chkAutoStash) chkAutoStash.checked = (currentWheelData.autoStashEnabled === true);



    const uiScaleSlider = document.getElementById('slider-ui-scale');
    const uiScaleTxt = document.getElementById('txt-ui-scale');
    const curUiScale = (currentWheelData && currentWheelData.uiScale) ? currentWheelData.uiScale : (parseFloat(safeGetStorage('outfit_wheeler_ui_scale', '1.0')) || 1.0);
    if (uiScaleSlider) uiScaleSlider.value = curUiScale;
    if (uiScaleTxt) uiScaleTxt.innerText = curUiScale.toFixed(2) + 'x';

    const curMode = (currentWheelData && currentWheelData.editUiMode !== undefined) ? currentWheelData.editUiMode : (parseInt(safeGetStorage('outfit_wheeler_edit_ui_mode', '0')) || 0);
    const btnClassic = document.getElementById('btn-mode-classic');
    const btnStudio = document.getElementById('btn-mode-studio');
    if (btnClassic && btnStudio) {
        if (curMode === 0) {
            btnClassic.classList.add('active');
            btnStudio.classList.remove('active');
        } else {
            btnStudio.classList.add('active');
            btnClassic.classList.remove('active');
        }
    }

    const storedEditKey = parseInt(safeGetStorage('outfit_wheeler_edit_key', ''), 10);
    const storedEditMod = parseInt(safeGetStorage('outfit_wheeler_edit_mod', ''), 10);
    const storedPlayKey = parseInt(safeGetStorage('outfit_wheeler_play_key', ''), 10);
    const storedPlayMod = parseInt(safeGetStorage('outfit_wheeler_play_mod', ''), 10);
    const storedGpKey = parseInt(safeGetStorage('outfit_wheeler_gp_play_key', ''), 10);
    const storedGpMod = parseInt(safeGetStorage('outfit_wheeler_gp_play_mod', ''), 10);

    tempEditKey = (currentWheelData && currentWheelData.editKey !== undefined && currentWheelData.editKey !== null) ? (parseInt(currentWheelData.editKey, 10) || 0x31) : (!isNaN(storedEditKey) ? storedEditKey : 0x31);
    tempEditMod = (currentWheelData && currentWheelData.editModifierKey !== undefined && currentWheelData.editModifierKey !== null) ? (parseInt(currentWheelData.editModifierKey, 10) || 0) : (!isNaN(storedEditMod) ? storedEditMod : 0);

    tempPlayKey = (currentWheelData && currentWheelData.playKey !== undefined && currentWheelData.playKey !== null) ? (parseInt(currentWheelData.playKey, 10) || 0x2F) : (!isNaN(storedPlayKey) ? storedPlayKey : 0x2F);
    tempPlayMod = (currentWheelData && currentWheelData.playModifierKey !== undefined && currentWheelData.playModifierKey !== null) ? (parseInt(currentWheelData.playModifierKey, 10) || 0) : (!isNaN(storedPlayMod) ? storedPlayMod : 0);

    tempGamepadPlayKey = (currentWheelData && currentWheelData.gamepadPlayKey !== undefined && currentWheelData.gamepadPlayKey !== null) ? (parseInt(currentWheelData.gamepadPlayKey, 10) || 0x0008) : (!isNaN(storedGpKey) ? storedGpKey : 0x0008);
    tempGamepadPlayMod = (currentWheelData && currentWheelData.gamepadPlayModifier !== undefined && currentWheelData.gamepadPlayModifier !== null) ? (parseInt(currentWheelData.gamepadPlayModifier, 10) || 0x0100) : (!isNaN(storedGpMod) ? storedGpMod : 0x0100);

    const editBtn = document.getElementById('btn-edit-key');
    const playBtn = document.getElementById('btn-play-key');
    const gpPlayBtn = document.getElementById('btn-gp-play-key');
    const editModText = document.getElementById('dd-edit-text');
    const playModText = document.getElementById('dd-play-text');
    const gpPlayModText = document.getElementById('dd-gp-play-text');

    if (editBtn) editBtn.innerText = '[' + getKeyName(tempEditKey) + ']';
    if (playBtn) playBtn.innerText = '[' + getKeyName(tempPlayKey) + ']';
    if (gpPlayBtn) gpPlayBtn.innerText = '[' + (gamepadNames[tempGamepadPlayKey] ? gamepadNames[tempGamepadPlayKey] : ('0x' + tempGamepadPlayKey.toString(16).toUpperCase())) + ']';

    if (editModText) editModText.innerText = (typeof modNameMap !== 'undefined' && modNameMap[tempEditMod]) ? modNameMap[tempEditMod] : (tempEditMod === 0 ? 'None' : 'Shift');
    if (playModText) playModText.innerText = (typeof modNameMap !== 'undefined' && modNameMap[tempPlayMod]) ? modNameMap[tempPlayMod] : (tempPlayMod === 0 ? 'None' : 'Shift');
    if (gpPlayModText) gpPlayModText.innerText = (typeof gpModNameMap !== 'undefined' && gpModNameMap[tempGamepadPlayMod]) ? gpModNameMap[tempGamepadPlayMod] : (tempGamepadPlayMod === 0 ? 'None' : 'LB');

    updateModPillUi('edit', tempEditMod);
    updateModPillUi('play', tempPlayMod);
    updateModPillUi('gp-play', tempGamepadPlayMod);

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
    const chkAutoSystem = document.getElementById('chk-auto-system');
    const autoSystemVal = chkAutoSystem ? chkAutoSystem.checked : false;

    const chkAutoClose = document.getElementById('chk-auto-close-equip');
    const autoCloseVal = chkAutoClose ? chkAutoClose.checked : true;

    const chkAutoStash = document.getElementById('chk-auto-stash');
    const autoStashVal = chkAutoStash ? chkAutoStash.checked : false;

    const uiScaleSlider = document.getElementById('slider-ui-scale');
    const savedUiScale = uiScaleSlider ? (parseFloat(uiScaleSlider.value) || 1.0) : 1.0;

    currentWheelData.autoSystemEnabled = autoSystemVal;
    currentWheelData.autoCloseOnEquip = autoCloseVal;
    currentWheelData.autoStashEnabled = autoStashVal;
    currentWheelData.autoCombatEnabled = true;
    currentWheelData.autoSwimEnabled = true;
    currentWheelData.autoCityEnabled = true;
    currentWheelData.autoHomeEnabled = true;
    currentWheelData.uiScale = savedUiScale;

    safeSetStorage('outfit_wheeler_ui_scale', String(savedUiScale));
    safeSetStorage('outfit_wheeler_edit_key', String(tempEditKey));
    safeSetStorage('outfit_wheeler_edit_mod', String(tempEditMod));
    safeSetStorage('outfit_wheeler_play_key', String(tempPlayKey));
    safeSetStorage('outfit_wheeler_play_mod', String(tempPlayMod));
    safeSetStorage('outfit_wheeler_gp_play_key', String(tempGamepadPlayKey));
    safeSetStorage('outfit_wheeler_gp_play_mod', String(tempGamepadPlayMod));
    applyAutoResolutionScale();

    currentWheelData.editKey = tempEditKey;
    currentWheelData.editModifierKey = tempEditMod;
    currentWheelData.playKey = tempPlayKey;
    currentWheelData.playModifierKey = tempPlayMod;
    currentWheelData.gamepadPlayKey = tempGamepadPlayKey;
    currentWheelData.gamepadPlayModifier = tempGamepadPlayMod;

    if (typeof saveWheelPositionToStorage === 'function') saveWheelPositionToStorage();

    if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            editMode: isEditModeActive(),
            wheelScale: currentWheelData.wheelScale || 1.0,
            wheelOffsetX: currentWheelData.wheelOffsetX || 0.0,
            wheelOffsetY: currentWheelData.wheelOffsetY || 0.0,
            uiScale: savedUiScale,
            editUiMode: (currentWheelData && currentWheelData.editUiMode !== undefined ? currentWheelData.editUiMode : 0),
            editKey: tempEditKey,
            editModifierKey: tempEditMod,
            playKey: tempPlayKey,
            playModifierKey: tempPlayMod,
            gamepadPlayKey: tempGamepadPlayKey,
            gamepadPlayModifier: tempGamepadPlayMod,
            autoCloseOnEquip: autoCloseVal,
            autoStashEnabled: autoStashVal,
            autoSystemEnabled: autoSystemVal,
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

    const currentSlot = (currentWheelData.slots || []).find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || (currentWheelData.slots || [])[selectedSlotIndex];
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
    if (tempStars === 1 && num === 1) {
        num = 0;
    }
    tempStars = num;
    document.querySelectorAll('#modal-stars-widget .star').forEach((s, idx) => {
        if (idx < num) s.classList.add('active');
        else s.classList.remove('active');
    });
    const label = document.getElementById('modal-stars-value-label');
    if (label) {
        label.innerText = num === 0 ? '(Excluded from Auto)' : '(' + num + ' Stars)';
    }
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
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: selectedSlotIndex,
            contextTag: contextString
        }));
    }

    if (typeof window.SetSlotRating === 'function' && currentWheelData) {
        window.SetSlotRating(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
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
        const slotIdx = parseInt(selectedSlotIndex, 10);
        const wheelIdx = parseInt(currentWheelData ? currentWheelData.wheelIndex : 0, 10) || 0;
        const realSlotIdx = isNaN(slotIdx) ? 0 : slotIdx;
        if (typeof window.RenameSlot === 'function') {
            window.RenameSlot(JSON.stringify({
                wheelIndex: wheelIdx,
                slotIndex: realSlotIdx,
                newName: val
            }));
        }
        const currentSlot = (currentWheelData && currentWheelData.slots) ? ((currentWheelData.slots || []).find(s => (s.index !== undefined ? s.index : 0) === realSlotIdx) || (currentWheelData.slots || [])[realSlotIdx]) : null;
        if (currentSlot) {
            currentSlot.name = val;
        }
        const drawerName = document.getElementById('drawer-outfit-name');
        if (drawerName) drawerName.innerText = val;
        if (typeof renderSlotsRail === 'function') renderSlotsRail();
        if (typeof renderDrawerDetails === 'function') renderDrawerDetails();
        if (typeof updateWheelUI === 'function') updateWheelUI();
    } else if (renameTargetMode === 'wheel') {
        const wheelIdx = parseInt(currentWheelData ? currentWheelData.wheelIndex : 0, 10) || 0;
        if (typeof window.RenameWheel === 'function') {
            window.RenameWheel(JSON.stringify({
                wheelIndex: wheelIdx,
                newName: val
            }));
        }
        if (currentWheelData) currentWheelData.wheelName = val;
        const wheelNameEl = document.getElementById('wheel-name');
        if (wheelNameEl) wheelNameEl.innerText = val;
        if (typeof updateWheelUI === 'function') updateWheelUI();
    }
    hideRenameModal();
}

function handleRenameKey(e) {
    if (e) e.stopPropagation();
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
    const currentSlot = (currentWheelData.slots || []).find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || (currentWheelData.slots || [])[selectedSlotIndex];
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
    if (!currentWheelData || (currentWheelData.totalWheels || 1) <= 1) return;

    const fromIdx = currentWheelData.wheelIndex || 0;
    const total = currentWheelData.totalWheels || 1;
    const toIdx = (fromIdx + delta + total) % total;

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
    const currentName = currentWheelData.wheelName || ('Wheel ' + ((currentWheelData.wheelIndex || 0) + 1));
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
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    const targetList = document.getElementById(type === 'edit' ? 'dd-edit-list' : (type === 'play' ? 'dd-play-list' : 'dd-gp-play-list'));
    if (!targetList) return;
    const isVisible = targetList.style.display === 'flex';
    closeAllDropdowns();
    if (!isVisible) {
        targetList.style.display = 'flex';
    }
}

function selectMod(type, val, text, e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    const numVal = parseInt(val, 10) || 0;
    if (type === 'edit') {
        tempEditMod = numVal;
        const el = document.getElementById('dd-edit-text');
        if (el) el.innerText = text;
        safeSetStorage('outfit_wheeler_edit_mod', String(numVal));
        if (currentWheelData) currentWheelData.editModifierKey = numVal;
    } else if (type === 'play') {
        tempPlayMod = numVal;
        const el = document.getElementById('dd-play-text');
        if (el) el.innerText = text;
        safeSetStorage('outfit_wheeler_play_mod', String(numVal));
        if (currentWheelData) currentWheelData.playModifierKey = numVal;
    } else if (type === 'gp-play') {
        tempGamepadPlayMod = numVal;
        const el = document.getElementById('dd-gp-play-text');
        if (el) el.innerText = text;
        safeSetStorage('outfit_wheeler_gp_play_mod', String(numVal));
        if (currentWheelData) currentWheelData.gamepadPlayModifier = numVal;
    }
    updateModPillUi(type, numVal);
    closeAllDropdowns();
}

function selectModPill(type, val, e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    const textMap = {
        0: 'None',
        42: 'Shift',
        29: 'Ctrl',
        56: 'Alt',
        256: 'LB',
        512: 'RB',
        9: 'LT',
        10: 'RT'
    };
    selectMod(type, val, textMap[val] || 'None', e);
}

function updateModPillUi(type, val) {
    const groupId = type === 'edit' ? 'group-edit-mod' : (type === 'play' ? 'group-play-mod' : 'group-gp-play-mod');
    const group = document.getElementById(groupId);
    if (!group) return;
    const pills = group.querySelectorAll('.mod-pill');
    pills.forEach(p => {
        const modVal = parseInt(p.getAttribute('data-mod'), 10) || 0;
        if (modVal === val) {
            p.classList.add('active');
        } else {
            p.classList.remove('active');
        }
    });
}

if (typeof window.closeAllDropdowns === 'undefined') {
    window.closeAllDropdowns = function() {
        ['dd-edit-list', 'dd-play-list', 'dd-gp-play-list'].forEach(id => {
            const el = document.getElementById(id);
            if (el) el.style.display = 'none';
        });
        const sortList = document.getElementById('custom-sort-list');
        const sortDropdown = document.getElementById('custom-sort-dropdown');
        if (sortList) sortList.style.display = 'none';
        if (sortDropdown) sortDropdown.classList.remove('open');
    };
}
var closeAllDropdowns = window.closeAllDropdowns;

function exportConfiguration() {
    if (window.Prisma && typeof window.Prisma.call === 'function') {
        window.Prisma.call('ExportConfig', '{}');
    } else {
        console.log('[OutfitWheeler] ExportConfig requested in test/mock mode');
    }
}

function importConfiguration() {
    if (window.Prisma && typeof window.Prisma.call === 'function') {
        window.Prisma.call('ImportConfig', JSON.stringify({ path: 'Data/SKSE/Plugins/PrismaOutfitWheel.json.bak' }));
    } else {
        console.log('[OutfitWheeler] ImportConfig requested in test/mock mode');
    }
}

if (typeof window !== 'undefined') {
    window.exportConfiguration = exportConfiguration;
    window.importConfiguration = importConfiguration;
}