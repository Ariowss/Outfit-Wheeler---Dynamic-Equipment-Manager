var tempScale = 1.0;
var tempOffsetX = 0.0;
var tempEditKey = 0x31;
var tempEditMod = 0;
var tempPlayKey = 0x2F;
var tempPlayMod = 0;
var tempGamepadPlayKey = 0x0008;
var tempGamepadPlayMod = 0x0100;
var tempStars = 3;
var renameTargetMode = 'slot';
var listeningKeyType = null;
var listeningGamepadType = null;
var toastTimeout = null;

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

function hideAllMenus(e) {
    if (e) e.stopPropagation();
    ['slot-context-menu', 'wheel-context-menu'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
}

function handleCenterRightClick(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }
    showWheelContextMenu(e.clientX || (window.innerWidth / 2), e.clientY || (window.innerHeight / 2));
}

function showWheelContextMenu(x, y) {
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
}

function closeWheel() {
    hideAllMenus();
    if (typeof window.OnCloseWheel === 'function') {
        window.OnCloseWheel("");
    }
}

function renderDrawerDetails() {
    const badge = document.getElementById('drawer-slot-badge');
    const nameEl = document.getElementById('drawer-outfit-name');
    const statusContainer = document.getElementById('drawer-slot-status');
    const btnFull = document.getElementById('btn-type-full');
    const btnAddon = document.getElementById('btn-type-addon');

    if (!badge || !nameEl || !currentWheelData || !currentWheelData.slots) return;

    const slot = currentWheelData.slots.find(s => s.index === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
    if (!slot) return;

    badge.innerText = 'SLOT ' + (selectedSlotIndex + 1);
    nameEl.innerText = slot.hasOutfit ? slot.name : 'Empty Slot';

    if (statusContainer) {
        if (slot.isAddon) {
            statusContainer.innerHTML = '<span class="status-chip chip-addon">ADDON</span>';
        } else if (slot.isWorn || slot.isEquipped) {
            statusContainer.innerHTML = '<span class="status-chip chip-equipped">EQUIPPED</span>';
        } else {
            statusContainer.innerHTML = '<span class="status-chip" style="background:rgba(255,255,255,0.06); color:#94a3b8;">INACTIVE</span>';
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

    updateStarsDisplay(slot.preferenceStars !== undefined ? slot.preferenceStars : 3);
    updateContextDisplay(slot.contextTag || '');
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
        row.innerHTML = `
            <div class="drawer-item-info">
                <span>${itemObj.icon || '🛡️'}</span>
                <span>${itemObj.name}</span>
            </div>
            <span class="drawer-item-slot-tag">${itemObj.type || 'Apparel'}</span>
        `;
        list.appendChild(row);
    });
}

function updateStarsDisplay(num) {
    const stars = document.querySelectorAll('#stars-widget .star-icon');
    stars.forEach((s, idx) => {
        if (idx < num) s.classList.add('active');
        else s.classList.remove('active');
    });
}

function updateContextDisplay(tagStr) {
    const lower = (tagStr || '').toLowerCase();
    const map = {
        'Combat': 'ctx-chip-combat',
        'City': 'ctx-chip-city',
        'Home': 'ctx-chip-home',
        'Swim': 'ctx-chip-swim'
    };
    for (let key in map) {
        const el = document.getElementById(map[key]);
        if (el) {
            if (lower.includes(key.toLowerCase())) el.classList.add('active');
            else el.classList.remove('active');
        }
    }
}

function toggleSlotAddonType(isAddon) {
    if (typeof window.SetSlotType === 'function' && currentWheelData) {
        window.SetSlotType(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            isAddon: isAddon
        }));
    }
    showSaveToast("Slot type updated!");
}

function setSlotRatingDirect(num) {
    if (typeof window.SetSlotRating === 'function' && currentWheelData) {
        window.SetSlotRating(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex,
            ratingStars: num
        }));
    }
    updateStarsDisplay(num);
    showSaveToast(num + " Stars Saved!");
}

function toggleContextTagDirect(ctxKey) {
    const slot = currentWheelData.slots.find(s => s.index === selectedSlotIndex);
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
    updateContextDisplay(newTagStr);
    showSaveToast("Context Updated: " + (newTagStr || "None"));
}

function saveCurrentEquippedToSlot() {
    saveOutfit(selectedSlotIndex);
}

function clearSelectedSlot() {
    ctxClearSlot();
}

function saveOutfit(slotIndex) {
    showSaveToast("Outfit Saved!");
    if (typeof window.SaveOutfit === 'function' && currentWheelData) {
        window.SaveOutfit(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: slotIndex
        }));
    }
}

function equipOutfit(slotIndex) {
    const now = Date.now();
    if (now - lastEquipCallTimestamp < 350) return;
    lastEquipCallTimestamp = now;

    lastEquippedSlotIndex = slotIndex;
    if (typeof updateWheelUI === 'function') updateWheelUI();
    if (typeof window.EquipSlot === 'function' && currentWheelData) {
        window.EquipSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: slotIndex,
            editMode: isEditModeActive()
        }));
    }
}

function changeWheel(delta) {
    if (typeof window.ChangeWheel === 'function' && currentWheelData) {
        window.ChangeWheel(JSON.stringify({
            delta: delta,
            editMode: isEditModeActive()
        }));
    }
}

function addWheel() {
    if (typeof window.AddWheel === 'function' && currentWheelData) {
        window.AddWheel(JSON.stringify({
            editMode: isEditModeActive()
        }));
    }
}

function addSlot() {
    if (typeof window.AddSlot === 'function' && currentWheelData) {
        window.AddSlot(JSON.stringify({
            editMode: isEditModeActive()
        }));
    }
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

function ctxToggleSlotWardrobe(e) {
    if (e) e.stopPropagation();
    if (selectedSlotIndex === null || !currentWheelData) return;
    if (typeof window.StashSlotOutfit === 'function') {
        window.StashSlotOutfit(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: selectedSlotIndex
        }));
    }
    showSaveToast("Transferred to Cloud!");
    hideAllMenus();
}

function openSettingsModal(e) {
    if (e) e.stopPropagation();
    hideAllMenus();
    const modal = document.getElementById('settings-modal');
    if (modal) modal.style.display = 'flex';
    setTyping(true);
}

function hideSettingsModal() {
    const modal = document.getElementById('settings-modal');
    if (modal) modal.style.display = 'none';
    setTyping(false);
}

function saveSettings() {
    const chkAutoStash = document.getElementById('chk-auto-stash');
    const chkMaster = document.getElementById('chk-auto-master');
    const chkCity = document.getElementById('chk-auto-city');
    const chkHome = document.getElementById('chk-auto-home');
    const chkCombat = document.getElementById('chk-auto-combat');
    const chkSwim = document.getElementById('chk-auto-swim');

    if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            editMode: isEditModeActive(),
            wheelScale: tempScale,
            wheelOffsetX: tempOffsetX,
            editKey: tempEditKey,
            editModifierKey: tempEditMod,
            playKey: tempPlayKey,
            playModifierKey: tempPlayMod,
            gamepadPlayKey: tempGamepadPlayKey,
            gamepadPlayModifier: tempGamepadPlayMod,
            autoStashEnabled: chkAutoStash ? chkAutoStash.checked : false,
            autoSystemEnabled: chkMaster ? chkMaster.checked : true,
            autoCityEnabled: chkCity ? chkCity.checked : true,
            autoHomeEnabled: chkHome ? chkHome.checked : true,
            autoCombatEnabled: chkCombat ? chkCombat.checked : true,
            autoSwimEnabled: chkSwim ? chkSwim.checked : true
        }));
    }
    hideSettingsModal();
}

function showRenameModal(titleText, initialValue) {
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
    const modal = document.getElementById('rename-modal');
    if (modal) modal.style.display = 'none';
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

function openContextModal(e) {
    if (e) e.stopPropagation();
    hideAllMenus();
    const modal = document.getElementById('context-modal');
    if (modal) modal.style.display = 'flex';
    setTyping(true);
}

function hideContextModal() {
    const modal = document.getElementById('context-modal');
    if (modal) modal.style.display = 'none';
    setTyping(false);
}

function setStarRating(num) {
    tempStars = num;
    document.querySelectorAll('#modal-star-rating .star').forEach((s, idx) => {
        if (idx < num) s.classList.add('active');
        else s.classList.remove('active');
    });
}

function saveSlotContextAndRating() {
    const chkCity = document.getElementById('chk-city');
    const chkHome = document.getElementById('chk-home');
    const chkCombat = document.getElementById('chk-combat');
    const chkSwim = document.getElementById('chk-swim');

    let tags = [];
    if (chkCity && chkCity.checked) tags.push("City");
    if (chkHome && chkHome.checked) tags.push("Home");
    if (chkCombat && chkCombat.checked) tags.push("Combat");
    if (chkSwim && chkSwim.checked) tags.push("Swim");

    const contextString = tags.join(", ");

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

    showSaveToast("Saved!");
    hideContextModal();
}

function stepScale(delta) {
    tempScale = Math.max(0.1, tempScale + delta);
    const inp = document.getElementById('scale-input');
    if (inp) inp.value = Math.round(tempScale * 100) + '%';
}

function stepOffsetX(delta) {
    tempOffsetX += delta;
    const inp = document.getElementById('offsetx-input');
    if (inp) inp.value = Math.round(tempOffsetX) + 'px';
}

function onScaleInputChange(val) {
    let num = parseFloat(String(val).replace('%', '').trim());
    if (!isNaN(num)) {
        if (num > 10) num /= 100.0;
        tempScale = Math.max(0.1, num);
    }
}

function onScaleInputBlur() { setTyping(false); }
function onOffsetXInputChange(val) {
    let num = parseFloat(String(val).replace('px', '').trim());
    if (!isNaN(num)) tempOffsetX = num;
}
function onOffsetXInputBlur() { setTyping(false); }

function openKeyboardModal(e) {
    if (e) e.stopPropagation();
    hideSettingsModal();
    const modal = document.getElementById('keyboard-modal');
    if (modal) modal.style.display = 'flex';
    setTyping(true);
}
function hideKeyboardModal() {
    const modal = document.getElementById('keyboard-modal');
    if (modal) modal.style.display = 'none';
    openSettingsModal();
}
function startKeyListen(type) {
    listeningKeyType = type;
    const btn = document.getElementById(type === 'edit' ? 'btn-edit-key' : 'btn-play-key');
    if (btn) btn.innerText = '[Press Any Key...]';
}
function saveKeyboardSettings() { hideKeyboardModal(); }

function openGamepadModal(e) {
    if (e) e.stopPropagation();
    hideSettingsModal();
    const modal = document.getElementById('gamepad-modal');
    if (modal) modal.style.display = 'flex';
    setTyping(true);
}
function hideGamepadModal() {
    const modal = document.getElementById('gamepad-modal');
    if (modal) modal.style.display = 'none';
    openSettingsModal();
}
function startGamepadListen(type) { listeningGamepadType = type; }
function saveGamepadSettings() { hideGamepadModal(); }