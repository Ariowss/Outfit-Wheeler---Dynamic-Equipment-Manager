var scanCodeMap = {
    'Escape': 0x01, 'Digit1': 0x02, 'Digit2': 0x03, 'Digit3': 0x04, 'Digit4': 0x05,
    'Digit5': 0x06, 'Digit6': 0x07, 'Digit7': 0x08, 'Digit8': 0x09, 'Digit9': 0x0A, 'Digit0': 0x0B,
    'Minus': 0x0C, 'Equal': 0x0D, 'Backspace': 0x0E, 'Tab': 0x0F,
    'KeyQ': 0x10, 'KeyW': 0x11, 'KeyE': 0x12, 'KeyR': 0x13, 'KeyT': 0x14, 'KeyY': 0x15,
    'KeyU': 0x16, 'KeyI': 0x17, 'KeyO': 0x18, 'KeyP': 0x19, 'BracketLeft': 0x1A, 'BracketRight': 0x1B,
    'Enter': 0x1C, 'ControlLeft': 0x1D,
    'KeyA': 0x1E, 'KeyS': 0x1F, 'KeyD': 0x20, 'KeyF': 0x21, 'KeyG': 0x22, 'KeyH': 0x23,
    'KeyJ': 0x24, 'KeyK': 0x25, 'KeyL': 0x26, 'Semicolon': 0x27, 'Quote': 0x28, 'Backquote': 0x29,
    'ShiftLeft': 0x2A, 'Backslash': 0x2B,
    'KeyZ': 0x2C, 'KeyX': 0x2D, 'KeyC': 0x2E, 'KeyV': 0x2F, 'KeyB': 0x30, 'KeyN': 0x31, 'KeyM': 0x32,
    'Comma': 0x33, 'Period': 0x34, 'Slash': 0x35, 'ShiftRight': 0x36, 'NumpadMultiply': 0x37,
    'AltLeft': 0x38, 'Space': 0x39, 'CapsLock': 0x3A,
    'F1': 0x3B, 'F2': 0x3C, 'F3': 0x3D, 'F4': 0x3E, 'F5': 0x3F,
    'F6': 0x40, 'F7': 0x41, 'F8': 0x42, 'F9': 0x43, 'F10': 0x44, 'F11': 0x57, 'F12': 0x58,
    'NumLock': 0x45, 'ScrollLock': 0x46,
    'Numpad7': 0x47, 'Numpad8': 0x48, 'Numpad9': 0x49, 'NumpadSubtract': 0x4A,
    'Numpad4': 0x4B, 'Numpad5': 0x4C, 'Numpad6': 0x4D, 'NumpadAdd': 0x4E,
    'Numpad1': 0x4F, 'Numpad2': 0x50, 'Numpad3': 0x51, 'Numpad0': 0x52, 'NumpadDecimal': 0x53,
    'ControlRight': 0x9D, 'AltRight': 0xB8,
    'Home': 0xC7, 'ArrowUp': 0xC8, 'PageUp': 0xC9, 'ArrowLeft': 0xCB,
    'ArrowRight': 0xCD, 'End': 0xCF, 'ArrowDown': 0xD0, 'PageDown': 0xD1, 'Insert': 0xD2, 'Delete': 0xD3
};

var gamepadNames = {
    0x0001: 'DPad Up', 0x0002: 'DPad Down', 0x0004: 'DPad Left', 0x0008: 'DPad Right',
    0x0010: 'Start', 0x0020: 'Back', 0x0040: 'L3', 0x0080: 'R3',
    0x0100: 'LB', 0x0200: 'RB', 0x1000: 'A', 0x2000: 'B', 0x4000: 'X', 0x8000: 'Y',
    0x0009: 'LT', 0x000A: 'RT'
};

var isShiftKeyDown = false;
window.addEventListener('keydown', (e) => {
    if (e.shiftKey || e.keyCode === 16 || e.key === 'Shift') {
        isShiftKeyDown = true;
    }
}, true);
window.addEventListener('keyup', (e) => {
    if (e.keyCode === 16 || e.key === 'Shift' || !e.shiftKey) {
        isShiftKeyDown = false;
    }
}, true);
window.addEventListener('blur', () => {
    isShiftKeyDown = false;
});

function safeClosest(target, selector) {
    if (!target) return null;
    let curr = (target.nodeType === 3) ? target.parentElement : target;
    while (curr && curr !== document && curr !== document.documentElement) {
        try {
            if (typeof curr.matches === 'function' && curr.matches(selector)) return curr;
            if (typeof curr.webkitMatchesSelector === 'function' && curr.webkitMatchesSelector(selector)) return curr;
        } catch (err) {}
        curr = curr.parentElement;
    }
    return null;
}

function safeGetStorage(key, defaultVal) {
    try {
        const val = localStorage.getItem(key);
        return (val !== null && val !== undefined) ? val : defaultVal;
    } catch (err) {
        return defaultVal;
    }
}

function safeSetStorage(key, val) {
    try {
        localStorage.setItem(key, String(val));
    } catch (err) {}
}

function safeRemoveStorage(key) {
    try {
        localStorage.removeItem(key);
    } catch (err) {}
}

window.safeClosest = safeClosest;
window.safeGetStorage = safeGetStorage;
window.safeSetStorage = safeSetStorage;
window.safeRemoveStorage = safeRemoveStorage;

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
var closeAllDropdowns = window.closeAllDropdowns;

var currentWheelData = {
    editMode: false,
    wheelIndex: 0,
    wheelName: "Wheel 1",
    totalWheels: 1,
    wheelScale: 1.0,
    wheelOffsetX: 0.0,
    wheelOffsetY: 0.0,
    activeTargetActorId: "0x00000007",
    editKey: 0x31,
    editModifierKey: 0,
    playKey: 0x2F,
    playModifierKey: 0,
    gamepadPlayKey: 0x0008,
    gamepadPlayModifier: 0x0100,
    gamepadConnected: false,
    autoCloseOnEquip: true,
    autoStashEnabled: true,
    autoCityEnabled: true,
    autoHomeEnabled: true,
    autoCombatEnabled: true,
    autoSwimEnabled: true,
    followers: [],
    inventory: [],
    actorInventories: {},
    playerInventory: [],
    followerInventory: [],
    cloudInventory: [],
    slots: []
};

var selectedSlotIndex = 0;
var inspectedItemObject = null;
var activeCategoryFilter = 'ALL';
var activeWheelContextFilter = 'ALL';
var activeInspectorContextTab = 'action';
var activeInventorySort = 'name_asc';
var activeInventorySource = '0x00000007';
var inventorySearchQuery = '';
var lastEquippedSlotIndex = null;
var lastWheelScrollTimestamp = 0;
var lastFollowerScrollTimestamp = 0;
var lastSourceScrollTimestamp = 0;
var lastEscTimestamp = 0;
var lastEquipCallTimestamp = 0;
var gamepadHoverSlotIndex = null;
var isTyping = false;
var listeningKeyType = null;
var listeningGamepadType = null;

var tempEditKey = 0x31;
var tempEditMod = 0;
var tempPlayKey = 0x2F;
var tempPlayMod = 0;
var tempGamepadPlayKey = 0x0008;
var tempGamepadPlayMod = 0x0100;
var tempScale = 1.0;
var tempOffsetX = 0.0;
var tempOffsetY = 0.0;

var isOrbitingCamera = false;
var lastCamMouseX = 0;
var lastCamMouseY = 0;

var isDraggingWheelCenter = false;
var wheelDragStartX = 0;
var wheelDragStartY = 0;
var wheelStartOffsetX = 0;
var wheelStartOffsetY = 0;
var wheelDragMoved = false;
var wheelDragButton = 0;

var activeDragPanelId = null;
var dragStartX = 0;
var dragStartY = 0;
var panelStartLeft = 0;
var panelStartTop = 0;
var panelRafId = null;

var activeResizePanelId = null;
var resizeDirection = '';
var resizeStartX = 0;
var resizeStartY = 0;
var panelStartWidth = 0;
var panelStartHeight = 0;
var resizeRafPending = false;
var resizeCurrentClientX = 0;
var resizeCurrentClientY = 0;
var uiScaleTimeout = null;
var currentAppliedZoom = null;

var activeDraggedItem = null;
var isItemDragging = false;
var dragGhostElement = null;
var dragItemStartX = 0;
var dragItemStartY = 0;

var activeDraggedSlotIndex = null;
var isSlotDragging = false;
var slotDragGhostElement = null;
var slotDragStartX = 0;
var slotDragStartY = 0;

document.addEventListener('DOMContentLoaded', () => {
    const wrapper = document.getElementById('ui-wrapper');
    if (wrapper) {
        wrapper.classList.remove('is-open');
        wrapper.style.display = 'none';
        wrapper.style.pointerEvents = 'none';
    }
    applyAutoResolutionScale();
    loadWheelPositionFromStorage();
    applyModeLayout();
    loadPanelsLayout();
    initDraggablePanels();
    initResizablePanels();
    initSlotsRailResizer();
    initCameraOrbitAndControls();
    initGlobalInputHandlers();
    initCarouselScrollHandlers();
    initWheelCenterInteraction();
    initCustomItemDragDrop();
    setupInventoryScrollChunking();
    if (typeof updateUiForPreviewsSetting === 'function') updateUiForPreviewsSetting();
    if (typeof updateWheelUI === 'function') updateWheelUI();
});

window.addEventListener('resize', () => {
    applyAutoResolutionScale();
    loadPanelsLayout();
    if (typeof updateWheelUI === 'function') updateWheelUI();
});

function getAutoResolutionScaleValue() {
    const h = (typeof window !== 'undefined' && window.screen && window.screen.height > 0) ? window.screen.height : ((typeof window !== 'undefined' && window.innerHeight) ? window.innerHeight : 1080);
    if (h >= 2000) return 1.75;
    if (h >= 1300) return 1.25;
    return 1.0;
}

function getUiScale() {
    if (currentWheelData && currentWheelData.uiScale !== undefined) {
        const s = parseFloat(currentWheelData.uiScale);
        if (!isNaN(s) && s >= 0.5 && s <= 3.0) return s;
    }
    const rawStored = typeof safeGetStorage === 'function' ? safeGetStorage('outfit_wheeler_ui_scale', '') : (typeof localStorage !== 'undefined' ? localStorage.getItem('outfit_wheeler_ui_scale') : null);
    const stored = parseFloat(rawStored);
    if (!isNaN(stored) && stored >= 0.5 && stored <= 3.0) return stored;
    return getAutoResolutionScaleValue();
}

function applyAutoResolutionScale(force) {
    const scale = getUiScale();
    const strScale = String(scale);
    if (!force && currentAppliedZoom === strScale) return;
    currentAppliedZoom = strScale;
    document.documentElement.style.zoom = strScale;
    document.documentElement.style.setProperty('--ui-scale', strScale);
    document.documentElement.style.setProperty('--auto-ui-scale', strScale);
    const wrapper = document.getElementById('ui-wrapper');
    if (wrapper) {
        wrapper.style.width = '';
        wrapper.style.height = '';
        wrapper.style.transform = '';
        wrapper.style.transformOrigin = '';
    }
}

function onUiScaleInput(val) {
    const userScale = parseFloat(val) || 1.0;
    const txt = document.getElementById('txt-ui-scale');
    if (txt) txt.innerText = userScale.toFixed(2) + 'x';
    localStorage.setItem('outfit_wheeler_ui_scale', String(userScale));
    if (currentWheelData) currentWheelData.uiScale = userScale;
    if (uiScaleTimeout) clearTimeout(uiScaleTimeout);
    uiScaleTimeout = setTimeout(() => {
        uiScaleTimeout = null;
        applyAutoResolutionScale(true);
    }, 40);
}

function onUiScaleChange(val) {
    if (uiScaleTimeout) {
        clearTimeout(uiScaleTimeout);
        uiScaleTimeout = null;
    }
    const userScale = parseFloat(val) || 1.0;
    localStorage.setItem('outfit_wheeler_ui_scale', String(userScale));
    if (currentWheelData) currentWheelData.uiScale = userScale;
    applyAutoResolutionScale(true);
    if (typeof persistWheelSettings === 'function') {
        persistWheelSettings();
    } else if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            uiScale: userScale
        }));
    }
}

function loadWheelPositionFromStorage() {
    try {
        const storedX = localStorage.getItem('outfit_wheeler_offset_x');
        const storedY = localStorage.getItem('outfit_wheeler_offset_y');
        const storedScale = localStorage.getItem('outfit_wheeler_scale');
        if (storedX !== null) {
            tempOffsetX = parseFloat(storedX) || 0.0;
            currentWheelData.wheelOffsetX = tempOffsetX;
        }
        if (storedY !== null) {
            tempOffsetY = parseFloat(storedY) || 0.0;
            currentWheelData.wheelOffsetY = tempOffsetY;
        }
        if (storedScale !== null) {
            tempScale = parseFloat(storedScale) || 1.0;
            currentWheelData.wheelScale = tempScale;
        }
    } catch (e) {}
}

function saveWheelPositionToStorage() {
    try {
        localStorage.setItem('outfit_wheeler_offset_x', String(currentWheelData.wheelOffsetX || 0.0));
        localStorage.setItem('outfit_wheeler_offset_y', String(currentWheelData.wheelOffsetY || 0.0));
        localStorage.setItem('outfit_wheeler_scale', String(currentWheelData.wheelScale || 1.0));
    } catch (e) {}
}

function isEditModeActive() {
    return currentWheelData && (currentWheelData.editMode === true || currentWheelData.editMode === "true" || currentWheelData.editMode === 1);
}

function isClassicEditMode() {
    if (!isEditModeActive()) return false;
    const mode = (currentWheelData && currentWheelData.editUiMode !== undefined) ? currentWheelData.editUiMode : (localStorage.getItem('outfit_wheeler_edit_ui_mode') !== null ? parseInt(localStorage.getItem('outfit_wheeler_edit_ui_mode')) : 1);
    return mode === 0;
}

function toggleEditUiMode(e) {
    if (e) {
        e.stopPropagation();
        e.preventDefault();
    }
    const currentMode = isClassicEditMode() ? 0 : 1;
    const newMode = currentMode === 0 ? 1 : 0;
    setEditUiStyle(newMode);
}

function setEditUiStyle(mode) {
    if (!currentWheelData) currentWheelData = {};
    currentWheelData.editUiMode = mode;
    localStorage.setItem('outfit_wheeler_edit_ui_mode', String(mode));

    const btnClassic = document.getElementById('btn-mode-classic');
    const btnStudio = document.getElementById('btn-mode-studio');
    if (btnClassic && btnStudio) {
        if (mode === 0) {
            btnClassic.classList.add('active');
            btnStudio.classList.remove('active');
        } else {
            btnStudio.classList.add('active');
            btnClassic.classList.remove('active');
        }
    }

    applyModeLayout();

    if (typeof window.SaveSettings === 'function') {
        window.SaveSettings(JSON.stringify({
            editMode: isEditModeActive(),
            wheelScale: currentWheelData.wheelScale || 1.0,
            wheelOffsetX: currentWheelData.wheelOffsetX || 0.0,
            wheelOffsetY: currentWheelData.wheelOffsetY || 0.0,
            uiScale: currentWheelData.uiScale || 1.0,
            editUiMode: mode,
            editKey: (currentWheelData.editKey !== undefined && currentWheelData.editKey !== null) ? currentWheelData.editKey : 0x31,
            editModifierKey: (currentWheelData.editModifierKey !== undefined && currentWheelData.editModifierKey !== null) ? (parseInt(currentWheelData.editModifierKey, 10) || 0) : 0,
            playKey: (currentWheelData.playKey !== undefined && currentWheelData.playKey !== null) ? currentWheelData.playKey : 0x2F,
            playModifierKey: (currentWheelData.playModifierKey !== undefined && currentWheelData.playModifierKey !== null) ? (parseInt(currentWheelData.playModifierKey, 10) || 0) : 0,
            gamepadPlayKey: (currentWheelData.gamepadPlayKey !== undefined && currentWheelData.gamepadPlayKey !== null) ? (parseInt(currentWheelData.gamepadPlayKey, 10) || 0x0008) : 0x0008,
            gamepadPlayModifier: (currentWheelData.gamepadPlayModifier !== undefined && currentWheelData.gamepadPlayModifier !== null) ? (parseInt(currentWheelData.gamepadPlayModifier, 10) || 0x0100) : 0x0100,
            autoCloseOnEquip: currentWheelData.autoCloseOnEquip !== false,
            autoStashEnabled: currentWheelData.autoStashEnabled === true,
            autoSystemEnabled: currentWheelData.autoSystemEnabled === true,
            autoCityEnabled: currentWheelData.autoCityEnabled !== false,
            autoHomeEnabled: currentWheelData.autoHomeEnabled !== false,
            autoCombatEnabled: currentWheelData.autoCombatEnabled !== false,
            autoSwimEnabled: currentWheelData.autoSwimEnabled !== false
        }));
    }

    if (typeof showSaveToast === 'function') {
        showSaveToast(mode === 0 ? "Switched to Classic Mode (v2.8)" : "Switched to Studio Mode (v3.0)");
    }
}

function updateModeSwitchButton() {
    const btn = document.getElementById('btn-ui-mode-switch');
    if (!btn) return;
    if (!isEditModeActive()) {
        btn.style.display = 'none';
        return;
    }
    btn.style.display = 'flex';
    if (isClassicEditMode()) {
        btn.innerText = 'Switch to Studio';
    } else {
        btn.innerText = 'Switch to Classic';
    }
}

function setTyping(state) {
    isTyping = state;
    const payload = JSON.stringify({ typing: state });
    if (typeof window.SetTypingMode === 'function') {
        window.SetTypingMode(payload);
    }
    if (typeof window.SetTypingState === 'function') {
        window.SetTypingState(payload);
    }
}

function sortOutfitsAlphabetically() {
    hideAllMenus();
    if (typeof window.SortOutfitsAlphabetically === 'function') {
        window.SortOutfitsAlphabetically('{}');
    }
    if (typeof showSaveToast === 'function') {
        showSaveToast('Outfits sorted alphabetically!');
    }
}
window.sortOutfitsAlphabetically = sortOutfitsAlphabetically;

function getScanCode(e) {
    if (e.code && scanCodeMap[e.code]) return scanCodeMap[e.code];
    const code = e.keyCode || e.which;
    if (code >= 112 && code <= 123) {
        const fKeyMap = { 112: 0x3B, 113: 0x3C, 114: 0x3D, 115: 0x3E, 116: 0x3F, 117: 0x40, 118: 0x41, 119: 0x42, 120: 0x43, 121: 0x44, 122: 0x57, 123: 0x58 };
        return fKeyMap[code] || 0x3B;
    }
    if (code >= 65 && code <= 90) {
        const letterMap = { 65: 0x1E, 66: 0x30, 67: 0x2E, 68: 0x20, 69: 0x12, 70: 0x21, 71: 0x22, 72: 0x23, 73: 0x17, 74: 0x24, 75: 0x25, 76: 0x26, 77: 0x32, 78: 0x31, 79: 0x18, 80: 0x19, 81: 0x10, 82: 0x13, 83: 0x1F, 84: 0x14, 85: 0x16, 86: 0x2F, 87: 0x11, 88: 0x2D, 89: 0x15, 90: 0x2C };
        return letterMap[code] || 0x31;
    }
    if (code >= 48 && code <= 57) {
        const digitMap = { 48: 0x0B, 49: 0x02, 50: 0x03, 51: 0x04, 52: 0x05, 53: 0x06, 54: 0x07, 55: 0x08, 56: 0x09, 57: 0x0A };
        return digitMap[code] || 0x02;
    }
    const specialMap = { 8: 0x0E, 9: 0x0F, 13: 0x1C, 16: 0x2A, 17: 0x1D, 18: 0x38, 20: 0x3A, 27: 0x01, 32: 0x39, 33: 0xC9, 34: 0xD1, 35: 0xCF, 36: 0xC7, 37: 0xC8, 39: 0xCD, 40: 0xD0, 45: 0xD2, 46: 0xD3, 186: 0x27, 187: 0x0D, 188: 0x33, 189: 0x0C, 190: 0x34, 191: 0x35, 192: 0x29, 219: 0x1A, 220: 0x2B, 221: 0x1B, 222: 0x28 };
    return specialMap[code] || 0x31;
}

function getKeyName(dxCode) {
    const customNames = {
        0x01: 'Esc', 0x0E: 'Backspace', 0x0F: 'Tab', 0x1C: 'Enter', 0x1D: 'LCtrl',
        0x2A: 'LShift', 0x38: 'LAlt', 0x39: 'Space', 0x3A: 'CapsLock', 0x36: 'RShift',
        0x9D: 'RCtrl', 0xB8: 'RAlt', 0xC7: 'Home', 0xC8: 'Up', 0xC9: 'PgUp',
        0xCB: 'Left', 0xCD: 'Right', 0xCF: 'End', 0xD0: 'Down', 0xD1: 'PgDn',
        0xD2: 'Insert', 0xD3: 'Delete'
    };
    if (customNames[dxCode]) return customNames[dxCode];
    for (let key in scanCodeMap) {
        if (scanCodeMap[key] === dxCode) return key.replace('Key', '').replace('Digit', '').replace('Arrow', '');
    }
    return '0x' + dxCode.toString(16).toUpperCase();
}

function hideAllMenus(e) {
    if (e && typeof e.stopPropagation === 'function') e.stopPropagation();
    const menus = ['slot-context-menu', 'wheel-context-menu', 'item-context-menu', 'transfer-all-context-menu', 'studio-pose-dropdown-menu'];
    menus.forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    const trigger = document.getElementById('btn-pose-dropdown-trigger');
    if (trigger) trigger.classList.remove('active');
    if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
}

function getElementUnderPoint(x, y) {
    if (typeof document === 'undefined' || typeof document.elementFromPoint !== 'function') return null;
    let el = document.elementFromPoint(x, y);
    if (!el || el === document.body || el === document.documentElement) {
        const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
        if (scale !== 1.0 && scale > 0) {
            const elScaled = document.elementFromPoint(x / scale, y / scale);
            if (elScaled && elScaled !== document.body && elScaled !== document.documentElement) {
                return elScaled;
            }
            const elMulti = document.elementFromPoint(x * scale, y * scale);
            if (elMulti && elMulti !== document.body && elMulti !== document.documentElement) {
                return elMulti;
            }
        }
    }
    return el;
}
window.getElementUnderPoint = getElementUnderPoint;

function positionContextMenu(menu, x, y) {
    if (!menu) return;
    menu.style.visibility = 'hidden';
    menu.style.display = 'flex';
    menu.style.maxHeight = '';
    menu.style.left = '0px';
    menu.style.top = '0px';

    const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
    const menuW = menu.offsetWidth || 180;
    const menuH = menu.offsetHeight || 260;

    const rawVw = window.innerWidth || (document.documentElement ? document.documentElement.clientWidth : 1920) || 1920;
    const rawVh = window.innerHeight || (document.documentElement ? document.documentElement.clientHeight : 1080) || 1080;
    const vw = rawVw / scale;
    const vh = rawVh / scale;

    let posX = (typeof x === 'number' && !isNaN(x) && x > 0) ? (x / scale) : (vw / 2);
    let posY = (typeof y === 'number' && !isNaN(y) && y > 0) ? (y / scale) : (vh / 2);

    if (posX + menuW > vw - 15) {
        posX = Math.max(15, posX - menuW);
    }
    if (posY + menuH > vh - 15) {
        if (posY - menuH >= 15) {
            posY = posY - menuH;
        } else {
            posY = Math.max(15, vh - menuH - 15);
        }
    }

    posX = Math.max(15, Math.min(posX, Math.max(15, vw - menuW - 15)));
    posY = Math.max(15, Math.min(posY, Math.max(15, vh - menuH - 15)));

    const maxH = Math.max(140, Math.round(vh - posY - 10));
    menu.style.maxHeight = maxH + 'px';
    menu.style.overflowY = 'auto';

    menu.style.left = Math.round(posX) + 'px';
    menu.style.top = Math.round(posY) + 'px';
    menu.style.visibility = 'visible';
}
window.positionContextMenu = positionContextMenu;

function isAnyModalOpen() {
    const modalIds = ['rename-modal', 'context-modal'];
    for (let id of modalIds) {
        const el = document.getElementById(id);
        if (el && el.style.display === 'flex') return true;
    }
    const panelSettings = document.getElementById('panel-settings');
    if (panelSettings && panelSettings.style.display !== 'none') return true;

    const slotCtx = document.getElementById('slot-context-menu');
    const wheelCtx = document.getElementById('wheel-context-menu');
    const itemCtx = document.getElementById('item-context-menu');
    const transferAllCtx = document.getElementById('transfer-all-context-menu');
    if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex') || (itemCtx && itemCtx.style.display === 'flex') || (transferAllCtx && transferAllCtx.style.display === 'flex')) return true;
    return false;
}

function updateGamepadHintsVisibility() {
    const hintsBar = document.getElementById('gamepad-hints-bar');
    if (!hintsBar) return;
    if (isEditModeActive()) {
        hintsBar.style.display = 'none';
        return;
    }
    let hasGp = currentWheelData && currentWheelData.gamepadConnected === true;
    if (!hasGp && typeof navigator.getGamepads === 'function') {
        const gps = navigator.getGamepads();
        if (gps) {
            for (let i = 0; i < gps.length; i++) {
                if (gps[i] && gps[i].connected) {
                    hasGp = true;
                    break;
                }
            }
        }
    }
    if (hasGp) {
        hintsBar.innerHTML = `
            <div class="gp-hint-item"><span class="gp-key">A</span><span>Equip</span></div>
            <div class="gp-hint-item"><span class="gp-key">B</span><span>Close</span></div>
            <div class="gp-hint-item"><span class="gp-key">Scroll / LB RB</span><span>Cycle Wheel</span></div>
            <div class="gp-hint-item"><span class="gp-key">LT / RT</span><span>Filter Context</span></div>
        `;
    } else {
        hintsBar.innerHTML = `
            <div class="gp-hint-item"><span class="gp-key">Scroll Wheel</span><span>Cycle Wheel</span></div>
            <div class="gp-hint-item"><span class="gp-key">LMB</span><span>Equip</span></div>
            <div class="gp-hint-item"><span class="gp-key">Shift + LMB</span><span>Audition</span></div>
            <div class="gp-hint-item"><span class="gp-key">Esc</span><span>Close</span></div>
        `;
    }
    hintsBar.style.display = 'flex';
}

function getInventoryBySourceId(sourceId) {
    if (!currentWheelData || !sourceId) return [];
    const srcUpper = sourceId.toUpperCase();
    if (srcUpper === 'CLOUD' || srcUpper === 'WARDROBE') {
        return currentWheelData.cloudInventory || [];
    }
    if (srcUpper === '0X00000007' || srcUpper === '0X00000014' || srcUpper === 'PLAYER') {
        return currentWheelData.playerInventory || currentWheelData.inventory || [];
    }
    if (currentWheelData.actorInventories) {
        for (let key in currentWheelData.actorInventories) {
            if (key.toUpperCase() === srcUpper) {
                return currentWheelData.actorInventories[key] || [];
            }
        }
    }
    if (srcUpper === 'MODEX' || srcUpper === 'CATALOG') {
        if (currentWheelData.catalogInventory) {
            return currentWheelData.catalogInventory;
        }
        let allCached = [];
        let seenKeys = new Set();
        for (let m in modCatalogCache) {
            const mLower = m.toLowerCase();
            if (!seenKeys.has(mLower) && Array.isArray(modCatalogCache[m])) {
                seenKeys.add(mLower);
                allCached = allCached.concat(modCatalogCache[m]);
            }
        }
        return allCached;
    }
    if (currentWheelData.followerInventory && currentWheelData.followerInventory.length > 0) {
        return currentWheelData.followerInventory;
    }
    return [];
}

var isOpenWheelRunning = false;
window.openWheel = function(argument) {
    if (isOpenWheelRunning) return;
    isOpenWheelRunning = true;
    try {
        var previousOffsetX = currentWheelData.wheelOffsetX;
        var previousOffsetY = currentWheelData.wheelOffsetY;
        var previousScale = currentWheelData.wheelScale;
        var prevInvSource = activeInventorySource;

        try {
            if (typeof argument === 'string' && argument.trim().length > 0) {
                currentWheelData = JSON.parse(argument);
            } else if (typeof argument === 'object' && argument !== null) {
                currentWheelData = argument;
            }
        } catch (e) {}

    if (!currentWheelData || typeof currentWheelData !== 'object') {
        currentWheelData = { slots: [], followers: [], inventory: [] };
    }

    if (currentWheelData.wheelOffsetX === undefined || (currentWheelData.wheelOffsetX === 0 && previousOffsetX !== undefined && previousOffsetX !== 0)) {
        currentWheelData.wheelOffsetX = previousOffsetX !== undefined ? previousOffsetX : tempOffsetX;
    }
    if (currentWheelData.wheelOffsetY === undefined || (currentWheelData.wheelOffsetY === 0 && previousOffsetY !== undefined && previousOffsetY !== 0)) {
        currentWheelData.wheelOffsetY = previousOffsetY !== undefined ? previousOffsetY : tempOffsetY;
    }
    if (currentWheelData.wheelScale === undefined || (currentWheelData.wheelScale === 1.0 && previousScale !== undefined && previousScale !== 1.0)) {
        currentWheelData.wheelScale = previousScale !== undefined ? previousScale : tempScale;
    }

    const storedUiScale = parseFloat(safeGetStorage('outfit_wheeler_ui_scale', ''));
    if (!isNaN(storedUiScale) && storedUiScale >= 0.5 && storedUiScale <= 3.0) {
        currentWheelData.uiScale = storedUiScale;
    } else if (currentWheelData.uiScale !== undefined && currentWheelData.uiScale >= 0.5 && currentWheelData.uiScale <= 3.0) {
        safeSetStorage('outfit_wheeler_ui_scale', String(currentWheelData.uiScale));
    } else {
        const autoScale = getAutoResolutionScaleValue();
        currentWheelData.uiScale = autoScale;
        safeSetStorage('outfit_wheeler_ui_scale', String(autoScale));
    }

    const storedInvSort = safeGetStorage('outfit_wheeler_inv_sort', '');
    if (storedInvSort) {
        activeInventorySort = storedInvSort;
        const storedSortLabel = safeGetStorage('outfit_wheeler_inv_sort_label', '');
        const sortLabelEl = document.getElementById('custom-sort-label');
        if (sortLabelEl && storedSortLabel) sortLabelEl.innerText = storedSortLabel;
        document.querySelectorAll('.custom-sort-list .sort-item').forEach(it => {
            if (it.getAttribute('data-sort') === storedInvSort) {
                it.classList.add('active');
            } else {
                it.classList.remove('active');
            }
        });
    }
    initSortDropdown();

    if (currentWheelData.editModifierKey !== undefined && currentWheelData.editModifierKey !== null) {
        currentWheelData.editModifierKey = parseInt(currentWheelData.editModifierKey, 10) || 0;
        safeSetStorage('outfit_wheeler_edit_mod', String(currentWheelData.editModifierKey));
    }

    if (currentWheelData.playModifierKey !== undefined && currentWheelData.playModifierKey !== null) {
        currentWheelData.playModifierKey = parseInt(currentWheelData.playModifierKey, 10) || 0;
        safeSetStorage('outfit_wheeler_play_mod', String(currentWheelData.playModifierKey));
    }

    if (currentWheelData.editKey !== undefined && currentWheelData.editKey !== null) {
        currentWheelData.editKey = parseInt(currentWheelData.editKey, 10) || 0x31;
        safeSetStorage('outfit_wheeler_edit_key', String(currentWheelData.editKey));
    }

    if (currentWheelData.playKey !== undefined && currentWheelData.playKey !== null) {
        currentWheelData.playKey = parseInt(currentWheelData.playKey, 10) || 0x2F;
        safeSetStorage('outfit_wheeler_play_key', String(currentWheelData.playKey));
    }

    if (currentWheelData.gamepadPlayKey !== undefined && currentWheelData.gamepadPlayKey !== null) {
        currentWheelData.gamepadPlayKey = parseInt(currentWheelData.gamepadPlayKey, 10) || 0x0008;
        safeSetStorage('outfit_wheeler_gp_play_key', String(currentWheelData.gamepadPlayKey));
    }

    if (currentWheelData.gamepadPlayModifier !== undefined && currentWheelData.gamepadPlayModifier !== null) {
        currentWheelData.gamepadPlayModifier = parseInt(currentWheelData.gamepadPlayModifier, 10) || 0x0100;
        safeSetStorage('outfit_wheeler_gp_play_mod', String(currentWheelData.gamepadPlayModifier));
    }

    if (currentWheelData.showItemPreviews !== undefined && currentWheelData.showItemPreviews !== null) {
        safeSetStorage('outfit_wheeler_show_previews', currentWheelData.showItemPreviews ? 'true' : 'false');
    }
    if (currentWheelData.themeAccentColor !== undefined && currentWheelData.themeAccentColor !== null) {
        safeSetStorage('outfit_wheeler_theme_hex', currentWheelData.themeAccentColor);
    }
    if (currentWheelData.themeBgColor !== undefined && currentWheelData.themeBgColor !== null) {
        safeSetStorage('outfit_wheeler_theme_bg', currentWheelData.themeBgColor);
    }
    if (currentWheelData.themeTextColor !== undefined && currentWheelData.themeTextColor !== null) {
        safeSetStorage('outfit_wheeler_theme_text', currentWheelData.themeTextColor);
    }
    if (currentWheelData.themeDepth !== undefined && currentWheelData.themeDepth !== null) {
        safeSetStorage('outfit_wheeler_theme_depth', String(currentWheelData.themeDepth));
    }
    if (currentWheelData.themePreset !== undefined && currentWheelData.themePreset !== null) {
        safeSetStorage('outfit_wheeler_theme_preset', currentWheelData.themePreset);
    }

    const themeHex = safeGetStorage('outfit_wheeler_theme_hex', currentWheelData.themeAccentColor || '#38bdf8');
    const themeBg = safeGetStorage('outfit_wheeler_theme_bg', currentWheelData.themeBgColor || '#070a0f');
    const themeText = safeGetStorage('outfit_wheeler_theme_text', currentWheelData.themeTextColor || '#f8fafc');
    const themeDepth = parseFloat(safeGetStorage('outfit_wheeler_theme_depth', String(currentWheelData.themeDepth !== undefined ? currentWheelData.themeDepth : 0.60)));
    const themePreset = safeGetStorage('outfit_wheeler_theme_preset', currentWheelData.themePreset || 'sapphire');
    if (typeof applyTheme === 'function') {
        applyTheme(themeHex, themeDepth, themePreset, themeBg, themeText);
    } else if (typeof window !== 'undefined' && typeof window.applyTheme === 'function') {
        window.applyTheme(themeHex, themeDepth, themePreset, themeBg, themeText);
    }

    tempOffsetX = currentWheelData.wheelOffsetX || 0.0;
    tempOffsetY = currentWheelData.wheelOffsetY || 0.0;
    tempScale = currentWheelData.wheelScale || 1.0;
    applyAutoResolutionScale();

    gamepadHoverSlotIndex = null;
    if (!activeWheelContextFilter) activeWheelContextFilter = 'ALL';
    activeCategoryFilter = 'ALL';

    const sources = getInventorySourcesList();
    if (prevInvSource && sources.some(s => s.id.toUpperCase() === prevInvSource.toUpperCase())) {
        activeInventorySource = prevInvSource;
    } else if (currentWheelData.activeTargetActorId && currentWheelData.activeTargetActorId.toUpperCase() !== 'CLOUD' && currentWheelData.activeTargetActorId.toUpperCase() !== 'WARDROBE') {
        const tgtUpper = currentWheelData.activeTargetActorId.toUpperCase();
        if (sources.some(s => s.id.toUpperCase() === tgtUpper)) {
            activeInventorySource = currentWheelData.activeTargetActorId;
        } else {
            activeInventorySource = '0x00000007';
        }
    } else {
        activeInventorySource = '0x00000007';
    }

    const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === activeWheelContextFilter) tab.classList.add('active');
        else tab.classList.remove('active');
    });

    if (isEditModeActive()) {
        const slots = currentWheelData.slots || [];
        const equippedSlot = slots.find(s => s.isEquipped === true || s.isWorn === true);
        if (equippedSlot && equippedSlot.index !== undefined) {
            selectedSlotIndex = equippedSlot.index;
        } else if (equippedSlot) {
            selectedSlotIndex = slots.indexOf(equippedSlot);
        } else if (slots.length > 0) {
            selectedSlotIndex = Math.max(0, Math.min(selectedSlotIndex, slots.length - 1));
        } else {
            selectedSlotIndex = 0;
        }
    }

    const panelSettings = document.getElementById('panel-settings');
    const isSettingsOpen = panelSettings && (panelSettings.style.display === 'flex' || panelSettings.style.display === 'block');
    hideAllMenus();
    if (typeof hideAllModals === 'function') hideAllModals(isSettingsOpen);
    if (!isSettingsOpen) setTyping(false);
    applyModeLayout();
    loadPanelsLayout();
    renderFollowersStrip();
    renderInventorySourceStrip();
    renderInventoryList();
    renderSlotsRail();
    renderDrawerDetails();
    if (typeof updateWheelUI === 'function') updateWheelUI();
    if (currentWheelData.posePacks && Array.isArray(currentWheelData.posePacks)) {
        populatePosePacks(currentWheelData.posePacks);
    }
    } catch (err) {
        console.error(err);
    } finally {
        isOpenWheelRunning = false;
        const wrapper = document.getElementById('ui-wrapper');
        if (wrapper) {
            wrapper.classList.add('is-open');
            wrapper.style.display = 'flex';
            wrapper.style.pointerEvents = 'auto';
            void wrapper.offsetHeight;
        }
        requestAnimationFrame(() => {
            updateVisibleThumbnailsPriority();
        });
    }
};

window.updateActiveWheel = function(argument) {
    try {
        let data = argument;
        if (typeof argument === 'string' && argument.trim().length > 0) {
            data = JSON.parse(argument);
        }
        if (!data || typeof data !== 'object') return;
        if (!currentWheelData) currentWheelData = {};

        currentWheelData.wheelIndex = (data.wheelIndex !== undefined) ? data.wheelIndex : currentWheelData.wheelIndex;
        currentWheelData.wheelName = data.wheelName || currentWheelData.wheelName;
        currentWheelData.wheelColor = data.wheelColor || '';
        currentWheelData.totalWheels = data.totalWheels || currentWheelData.totalWheels;
        if (Array.isArray(data.slots)) {
            currentWheelData.slots = data.slots;
        }

        const titleEl = document.getElementById('wheel-title-display');
        if (titleEl) {
            const wNum = (currentWheelData.wheelIndex !== undefined ? currentWheelData.wheelIndex : 0) + 1;
            const wTot = currentWheelData.totalWheels || 1;
            const wName = currentWheelData.wheelName || ('Wheel ' + wNum);
            titleEl.textContent = `${wName} (${wNum}/${wTot})`;
        }

        selectedSlotIndex = 0;
        if (typeof updateWheelUI === 'function') updateWheelUI();
        if (typeof renderSlotsRail === 'function') renderSlotsRail();
        if (typeof renderDrawerDetails === 'function') renderDrawerDetails();
    } catch (err) {
        console.error('updateActiveWheel error:', err);
    }
};

window.setGamepadAngle = function(phiStr) {
    const phi = parseFloat(phiStr);
    if (isNaN(phi)) return;

    const paths = document.querySelectorAll('.slice-path');
    const total = paths.length;
    if (total === 0) return;

    const sliceAngle = (2 * Math.PI) / total;
    let normalizedPhi = phi + (sliceAngle / 2);
    if (normalizedPhi < 0) normalizedPhi += (2 * Math.PI);
    normalizedPhi = normalizedPhi % (2 * Math.PI);

    let targetIdx = Math.floor(normalizedPhi / sliceAngle);
    if (targetIdx >= total) targetIdx = total - 1;
    if (targetIdx < 0) targetIdx = 0;

    if (targetIdx !== gamepadHoverSlotIndex) {
        gamepadHoverSlotIndex = targetIdx;
        paths.forEach((p, idx) => {
            if (idx === targetIdx) {
                p.classList.add('gamepad-hover');
            } else {
                p.classList.remove('gamepad-hover');
            }
        });

        if (isEditModeActive() && paths[targetIdx]) {
            const realIdx = parseInt(paths[targetIdx].getAttribute('data-slot-index'), 10);
            if (!isNaN(realIdx)) {
                selectedSlotIndex = realIdx;
                renderSlotsRail();
                renderDrawerDetails();
            }
        }
    }
};

window.onGamepadButtonPress = function(codeStr) {
    const code = parseInt(codeStr);
    if (isNaN(code)) return;

    if (listeningGamepadType === 'play') {
        tempGamepadPlayKey = code;
        const btn = document.getElementById('btn-gp-play-key');
        if (btn) {
            btn.innerText = '[' + (gamepadNames[code] ? gamepadNames[code] : ('0x' + code.toString(16).toUpperCase())) + ']';
            btn.classList.remove('listening');
        }
        listeningGamepadType = null;
    }
};

window.gamepadConfirm = function() {
    if (isAnyModalOpen()) return;

    const paths = document.querySelectorAll('.slice-path');
    if (gamepadHoverSlotIndex !== null && gamepadHoverSlotIndex < paths.length) {
        const targetPath = paths[gamepadHoverSlotIndex];
        if (targetPath) {
            const realIdx = parseInt(targetPath.getAttribute('data-slot-index'));
            if (!isNaN(realIdx)) {
                if (isEditModeActive()) saveOutfit(realIdx);
                else equipOutfit(realIdx, currentWheelData.autoCloseOnEquip === false);
            }
        }
    }
};

window.gamepadCycleWheel = function(deltaStr) {
    if (isAnyModalOpen()) return;
    const delta = parseInt(deltaStr) || 1;
    changeWheel(delta);
};

window.gamepadCycleFilter = function(deltaStr) {
    if (isAnyModalOpen()) return;
    const delta = parseInt(deltaStr) || 1;
    const tags = ['ALL', 'City', 'Home', 'Combat', 'Swim'];
    let currentIdx = tags.indexOf(activeWheelContextFilter);
    if (currentIdx === -1) currentIdx = 0;
    currentIdx = (currentIdx + delta + tags.length) % tags.length;
    setWheelContextFilter(tags[currentIdx]);
};

function startKeyListen(type) {
    listeningKeyType = type;
    listeningGamepadType = null;
    const btn = document.getElementById(type === 'edit' ? 'btn-edit-key' : 'btn-play-key');
    if (btn) {
        btn.innerText = "[Press Key...]";
        btn.classList.add('listening');
        btn.blur();
    }
}

function startGamepadListen(type) {
    listeningGamepadType = type;
    listeningKeyType = null;
    const btn = document.getElementById('btn-gp-play-key');
    if (btn) {
        btn.innerText = "[Press Button...]";
        btn.classList.add('listening');
        btn.blur();
    }
}

function applyModeLayout() {
    const wrapper = document.getElementById('ui-wrapper');
    const centerModeText = document.getElementById('mode-text');
    if (!wrapper) return;

    if (isEditModeActive()) {
        wrapper.classList.remove('play-mode-active');
        if (isClassicEditMode()) {
            wrapper.classList.add('classic-ui-active');
            wrapper.classList.remove('studio-ui-active');
            if (centerModeText) centerModeText.innerText = "EDIT MODE";
        } else {
            wrapper.classList.remove('classic-ui-active');
            wrapper.classList.add('studio-ui-active');
            if (centerModeText) centerModeText.innerText = "EDIT MODE";
        }
    } else {
        wrapper.classList.add('play-mode-active');
        wrapper.classList.remove('classic-ui-active');
        wrapper.classList.remove('studio-ui-active');
        if (centerModeText) centerModeText.innerText = "PLAY MODE";
    }
    updateModeSwitchButton();
    updateGamepadHintsVisibility();
}

function handleWrapperClick(e) {
    if (!e || !e.target) return;
    let target = e.target;
    while (target && target !== document.body && target !== document.documentElement) {
        if (target.classList) {
            if (target.classList.contains('context-menu-popup') ||
                target.classList.contains('custom-sort-dropdown') ||
                target.classList.contains('custom-sort-list') ||
                target.classList.contains('modal-card') ||
                target.classList.contains('modifier-pill-group') ||
                target.classList.contains('custom-dropdown') ||
                target.classList.contains('dropdown-list') ||
                target.classList.contains('studio-pose-bar') ||
                target.classList.contains('studio-pose-dropdown-menu') ||
                target.classList.contains('pose-bar-inner') ||
                target.classList.contains('pose-menu-item')) {
                return;
            }
        }
        if (target.id === 'panel-settings' || target.id === 'studio-pose-bar' || target.id === 'studio-pose-dropdown-menu') return;
        target = target.parentElement;
    }
    hideAllMenus();
    if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
}

function handleEscClose(e) {
    if (e.key === 'Escape' || e.code === 'Escape' || e.keyCode === 27) {
        e.preventDefault();
        e.stopPropagation();

        const now = Date.now();
        if (now - lastEscTimestamp < 180) return true;
        lastEscTimestamp = now;

        if (listeningKeyType || listeningGamepadType) {
            listeningKeyType = null;
            listeningGamepadType = null;
            const editBtn = document.getElementById('btn-edit-key');
            const playBtn = document.getElementById('btn-play-key');
            const gpPlayBtn = document.getElementById('btn-gp-play-key');
            if (editBtn) {
                editBtn.innerText = '[' + getKeyName(tempEditKey) + ']';
                editBtn.classList.remove('listening');
            }
            if (playBtn) {
                playBtn.innerText = '[' + getKeyName(tempPlayKey) + ']';
                playBtn.classList.remove('listening');
            }
            if (gpPlayBtn) {
                gpPlayBtn.innerText = '[' + (gamepadNames[tempGamepadPlayKey] ? gamepadNames[tempGamepadPlayKey] : ('0x' + tempGamepadPlayKey.toString(16).toUpperCase())) + ']';
                gpPlayBtn.classList.remove('listening');
            }
            return true;
        }

        const ddEdit = document.getElementById('dd-edit-list');
        const ddPlay = document.getElementById('dd-play-list');
        const ddGpPlay = document.getElementById('dd-gp-play-list');
        const sortList = document.getElementById('custom-sort-list');
        if ((ddEdit && ddEdit.style.display === 'flex') ||
            (ddPlay && ddPlay.style.display === 'flex') ||
            (ddGpPlay && ddGpPlay.style.display === 'flex') ||
            (sortList && sortList.style.display === 'flex')) {
            if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
            hideAllMenus();
            return true;
        }

        const panelSettings = document.getElementById('panel-settings');
        if (panelSettings && panelSettings.style.display !== 'none') {
            hideSettingsModal();
            return true;
        }

        const renameModal = document.getElementById('rename-modal');
        if (renameModal && renameModal.style.display === 'flex') {
            if (typeof hideRenameModal === 'function') hideRenameModal();
            return true;
        }

        const ctxModal = document.getElementById('context-modal');
        if (ctxModal && ctxModal.style.display === 'flex') {
            if (typeof hideContextModal === 'function') hideContextModal();
            return true;
        }

        const slotCtx = document.getElementById('slot-context-menu');
        const wheelCtx = document.getElementById('wheel-context-menu');
        const itemCtx = document.getElementById('item-context-menu');
        const transferAllCtx = document.getElementById('transfer-all-context-menu');
        if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex') || (itemCtx && itemCtx.style.display === 'flex') || (transferAllCtx && transferAllCtx.style.display === 'flex')) {
            hideAllMenus();
            return true;
        }

        closeWheel();
        return true;
    }
    return false;
}

function initGlobalInputHandlers() {
    window.addEventListener('keydown', handleEscClose, true);

    window.addEventListener('contextmenu', (e) => {
        e.preventDefault();
    });

    document.addEventListener('focusin', (e) => {
        if (e.target && (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.isContentEditable)) {
            setTyping(true);
        }
    });

    document.addEventListener('focusout', (e) => {
        if (e.target && (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.isContentEditable)) {
            setTyping(false);
        }
    });

    document.addEventListener('wheel', (e) => {
        let rawDelta = 0;
        if (typeof e.deltaY === 'number' && e.deltaY !== 0) {
            rawDelta = e.deltaY;
        } else if (typeof e.wheelDelta === 'number' && e.wheelDelta !== 0) {
            rawDelta = -e.wheelDelta;
        } else if (typeof e.detail === 'number' && e.detail !== 0) {
            rawDelta = e.detail * 40;
        }
        if (!rawDelta) return;

        if (safeClosest(e.target, '.draggable-panel') ||
            safeClosest(e.target, '.glass-panel') ||
            safeClosest(e.target, '#panel-settings') ||
            safeClosest(e.target, '#panel-inventory') ||
            safeClosest(e.target, '#panel-drawer') ||
            safeClosest(e.target, '#right-dock-container') ||
            safeClosest(e.target, '#studio-pose-bar') ||
            safeClosest(e.target, '.studio-pose-dropdown-menu') ||
            safeClosest(e.target, '.panel-body-scrollable') ||
            safeClosest(e.target, '#inventory-body-scrollable') ||
            safeClosest(e.target, '#drawer-body-scrollable') ||
            safeClosest(e.target, '.custom-sort-list') ||
            safeClosest(e.target, '.modal-card') ||
            safeClosest(e.target, '.modal-overlay') ||
            safeClosest(e.target, '.context-menu-popup') ||
            safeClosest(e.target, '.slots-rail-list') ||
            safeClosest(e.target, '.color-picker-dropdown')) {
            return;
        }

        if (isDraggingWheelCenter && wheelDragButton === 0) return;
        if (isEditModeActive() && !safeClosest(e.target, '#wheel-container') && !safeClosest(e.target, '.wheel-center')) return;
        const now = Date.now();
        if (now - lastWheelScrollTimestamp < 150) return;
        lastWheelScrollTimestamp = now;

        if (currentWheelData) {
            const delta = rawDelta < 0 ? -1 : 1;
            changeWheel(delta);
        }
    }, { passive: true });

    document.addEventListener('keydown', (e) => {
        if (listeningKeyType) {
            e.preventDefault();
            e.stopPropagation();

            if (e.keyCode === 16 || e.keyCode === 17 || e.keyCode === 18 || e.key === 'Shift' || e.key === 'Control' || e.key === 'Alt') {
                let modCode = 42;
                if (e.keyCode === 17 || e.key === 'Control') modCode = 29;
                else if (e.keyCode === 18 || e.key === 'Alt') modCode = 56;

                if (listeningKeyType === 'edit') {
                    tempEditMod = modCode;
                    safeSetStorage('outfit_wheeler_edit_mod', String(modCode));
                    if (currentWheelData) currentWheelData.editModifierKey = modCode;
                    const editModText = document.getElementById('dd-edit-text');
                    if (editModText) editModText.innerText = (typeof modNameMap !== 'undefined' && modNameMap[modCode]) ? modNameMap[modCode] : 'Shift';
                    if (typeof updateModPillUi === 'function') updateModPillUi('edit', modCode);
                    const btn = document.getElementById('btn-edit-key');
                    if (btn) btn.innerText = '[' + ((typeof modNameMap !== 'undefined' && modNameMap[modCode]) ? modNameMap[modCode] : 'Shift') + ' + ...]';
                } else if (listeningKeyType === 'play') {
                    tempPlayMod = modCode;
                    safeSetStorage('outfit_wheeler_play_mod', String(modCode));
                    if (currentWheelData) currentWheelData.playModifierKey = modCode;
                    const playModText = document.getElementById('dd-play-text');
                    if (playModText) playModText.innerText = (typeof modNameMap !== 'undefined' && modNameMap[modCode]) ? modNameMap[modCode] : 'Shift';
                    if (typeof updateModPillUi === 'function') updateModPillUi('play', modCode);
                    const btn = document.getElementById('btn-play-key');
                    if (btn) btn.innerText = '[' + ((typeof modNameMap !== 'undefined' && modNameMap[modCode]) ? modNameMap[modCode] : 'Shift') + ' + ...]';
                }
                return;
            }

            const code = getScanCode(e);
            if (code) {
                let comboMod = 0;
                if (e.shiftKey) comboMod = 42;
                else if (e.ctrlKey) comboMod = 29;
                else if (e.altKey) comboMod = 56;

                if (listeningKeyType === 'edit') {
                    tempEditKey = code;
                    if (comboMod !== 0) {
                        tempEditMod = comboMod;
                        const editModText = document.getElementById('dd-edit-text');
                        if (editModText) editModText.innerText = (typeof modNameMap !== 'undefined' && modNameMap[comboMod]) ? modNameMap[comboMod] : 'None';
                        if (typeof updateModPillUi === 'function') updateModPillUi('edit', comboMod);
                    }
                    const btn = document.getElementById('btn-edit-key');
                    if (btn) {
                        btn.innerText = '[' + getKeyName(code) + ']';
                        btn.classList.remove('listening');
                    }
                } else if (listeningKeyType === 'play') {
                    tempPlayKey = code;
                    if (comboMod !== 0) {
                        tempPlayMod = comboMod;
                        const playModText = document.getElementById('dd-play-text');
                        if (playModText) playModText.innerText = (typeof modNameMap !== 'undefined' && modNameMap[comboMod]) ? modNameMap[comboMod] : 'None';
                        if (typeof updateModPillUi === 'function') updateModPillUi('play', comboMod);
                    }
                    const btn = document.getElementById('btn-play-key');
                    if (btn) {
                        btn.innerText = '[' + getKeyName(code) + ']';
                        btn.classList.remove('listening');
                    }
                }
            }
            listeningKeyType = null;
            return;
        }

        if (isTyping || (document.activeElement && (document.activeElement.tagName === 'INPUT' || document.activeElement.tagName === 'TEXTAREA' || document.activeElement.isContentEditable))) {
            return;
        }
    });

    document.addEventListener('mousedown', (e) => {
        const isInsideMenu = !!safeClosest(e.target, '.context-menu-popup, .studio-pose-dropdown-menu, .studio-pose-bar, .pose-menu-item');
        const isInsideDropdown = !!safeClosest(e.target, '.custom-dropdown, .custom-sort-dropdown, .dropdown-list, .custom-sort-list, #panel-settings, .modifier-pill-group, .studio-pose-bar, .studio-pose-dropdown-menu');

        if (e.button === 0) {
            if (!isInsideMenu) {
                hideAllMenus();
            }
            if (!isInsideDropdown) {
                closeAllDropdowns();
            }
            return;
        }

        if (e.button === 2 || e.which === 3) {
            activeDraggedItem = null;
            isItemDragging = false;
            if (dragGhostElement) {
                dragGhostElement.remove();
                dragGhostElement = null;
            }

            activeDraggedSlotIndex = null;
            isSlotDragging = false;
            if (slotDragGhostElement) {
                slotDragGhostElement.remove();
                slotDragGhostElement = null;
            }

            if (isInsideMenu || safeClosest(e.target, '.modal-card') || safeClosest(e.target, '#panel-settings')) return;

            const itemElem = safeClosest(e.target, '.item-row-card, .item-card');
            if (itemElem) {
                e.preventDefault();
                e.stopPropagation();

                const itemMenu = document.getElementById('item-context-menu');
                const rawIdStr = itemElem.getAttribute('data-raw-id');
                const formIdStr = itemElem.getAttribute('data-form-id');
                const uniqueIdStr = itemElem.getAttribute('data-unique-id');
                const inv = getInventoryBySourceId(activeInventorySource);
                let matchedItem = itemElem._item || null;
                if (!matchedItem && uniqueIdStr !== null && uniqueIdStr !== '0' && uniqueIdStr !== '') {
                    const uId = parseInt(uniqueIdStr, 10);
                    matchedItem = inv.find(x => x.uniqueID === uId);
                }
                if (!matchedItem && inspectedItemObject) {
                    const inspectedRawId = inspectedItemObject.rawID || inspectedItemObject.id;
                    if ((rawIdStr && String(inspectedRawId) === rawIdStr) || (formIdStr && inspectedItemObject.id === formIdStr)) {
                        matchedItem = inspectedItemObject;
                    }
                }
                if (!matchedItem && rawIdStr !== null && rawIdStr !== '0') {
                    const rId = parseInt(rawIdStr, 10);
                    const candidates = inv.filter(x => x.rawID === rId);
                    if (candidates.length === 1) {
                        matchedItem = candidates[0];
                    } else if (candidates.length > 1) {
                        const itemNameEl = itemElem.querySelector('.item-name-text, .card-name-label');
                        const itemName = itemNameEl ? itemNameEl.innerText.trim() : '';
                        matchedItem = candidates.find(x => (x.customName && x.customName === itemName) || x.name === itemName) || candidates[0];
                    }
                }
                if (!matchedItem && formIdStr) {
                    matchedItem = inv.find(x => x.id === formIdStr);
                }
                if (!matchedItem) {
                    const itemNameEl = itemElem.querySelector('.item-name-text, .card-name-label');
                    const itemName = itemNameEl ? itemNameEl.innerText.trim() : '';
                    matchedItem = inv.find(x => x.name === itemName);
                }
                if (!matchedItem) {
                    matchedItem = inspectedItemObject;
                }

                if (itemMenu && itemMenu.style.display === 'flex' && inspectedItemObject && matchedItem && (inspectedItemObject === matchedItem || (inspectedItemObject.uniqueID && matchedItem.uniqueID && inspectedItemObject.uniqueID === matchedItem.uniqueID) || (inspectedItemObject.rawID && matchedItem.rawID && inspectedItemObject.rawID === matchedItem.rawID))) {
                    hideAllMenus();
                    return;
                }

                if (matchedItem) {
                    showItemContextMenu(e.clientX, e.clientY, matchedItem);
                }
                return;
            }

            const slotElem = safeClosest(e.target, '[data-slot-index]');
            if (slotElem) {
                const idx = parseInt(slotElem.getAttribute('data-slot-index'), 10);
                if (!isNaN(idx)) {
                    e.preventDefault();
                    e.stopPropagation();

                    const slotMenu = document.getElementById('slot-context-menu');
                    if (slotMenu && slotMenu.style.display === 'flex' && selectedSlotIndex === idx) {
                        hideAllMenus();
                        return;
                    }

                    const rect = slotElem.getBoundingClientRect();
                    const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (rect.left + rect.width / 2);
                    const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (rect.top + rect.height / 2);
                    if (typeof showSlotContextMenu === 'function') showSlotContextMenu(clickX, clickY, idx);
                    return;
                }
            }

            const panelElem = safeClosest(e.target, '.draggable-panel, #panel-settings, .wheel-center');
            if (panelElem) {
                return;
            }

            if (isEditModeActive()) {
                e.preventDefault();
                e.stopPropagation();

                const wheelMenu = document.getElementById('wheel-context-menu');
                if (wheelMenu && wheelMenu.style.display === 'flex') {
                    hideAllMenus();
                    return;
                }

                hideAllMenus();
                const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (window.innerWidth / 2);
                const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (window.innerHeight / 2);
                if (typeof showWheelContextMenu === 'function') showWheelContextMenu(clickX, clickY);
                return;
            } else {
                if (e.target.id === 'ui-wrapper' || e.target.id === 'wheel-container' || e.target.id === 'wheel-svg' || safeClosest(e.target, '.wheel-center')) {
                    e.preventDefault();
                    e.stopPropagation();

                    const wheelMenu = document.getElementById('wheel-context-menu');
                    if (wheelMenu && wheelMenu.style.display === 'flex') {
                        hideAllMenus();
                        return;
                    }

                    hideAllMenus();
                    const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (window.innerWidth / 2);
                    const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (window.innerHeight / 2);
                    if (typeof showWheelContextMenu === 'function') showWheelContextMenu(clickX, clickY);
                }
            }
        }
    }, false);

    window.addEventListener('gamepadconnected', () => { updateGamepadHintsVisibility(); });
    window.addEventListener('gamepaddisconnected', () => { updateGamepadHintsVisibility(); });
}

function toggleSortDropdown(e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    const list = document.getElementById('custom-sort-list');
    const dropdown = document.getElementById('custom-sort-dropdown');
    if (!list || !dropdown) return;

    const isOpen = list.style.display === 'flex';
    closeAllDropdowns();
    if (!isOpen) {
        list.style.display = 'flex';
        dropdown.classList.add('open');
    } else {
        list.style.display = 'none';
        dropdown.classList.remove('open');
    }
}

function selectSortOption(val, elemOrLabel, e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    let labelText = '';
    if (typeof elemOrLabel === 'string') {
        labelText = elemOrLabel;
    } else if (elemOrLabel && elemOrLabel.textContent) {
        labelText = elemOrLabel.textContent.trim();
    } else if (elemOrLabel && elemOrLabel.innerText) {
        labelText = elemOrLabel.innerText.trim();
    }
    if (!val && elemOrLabel && elemOrLabel.getAttribute) {
        val = elemOrLabel.getAttribute('data-sort');
    }
    if (!val) return;

    activeInventorySort = val;
    safeSetStorage('outfit_wheeler_inv_sort', val);

    const items = document.querySelectorAll('.custom-sort-list .sort-item');
    items.forEach(it => {
        if (it.getAttribute('data-sort') === val) {
            it.classList.add('active');
            if (!labelText) {
                labelText = (it.textContent || it.innerText || '').trim();
            }
        } else {
            it.classList.remove('active');
        }
    });

    if (labelText) {
        safeSetStorage('outfit_wheeler_inv_sort_label', labelText);
        const labelEl = document.getElementById('custom-sort-label');
        if (labelEl) labelEl.innerText = labelText;
    }

    closeAllDropdowns();
    renderInventoryList();
}

function initSortDropdown() {
    const list = document.getElementById('custom-sort-list');
    if (!list) return;
    list.querySelectorAll('.sort-item').forEach(item => {
        const handler = function(e) {
            if (e) {
                if (typeof e.preventDefault === 'function') e.preventDefault();
                if (typeof e.stopPropagation === 'function') e.stopPropagation();
            }
            const sortKey = this.getAttribute('data-sort');
            selectSortOption(sortKey, this, e);
        };
        item.onclick = handler;
        item.onmousedown = handler;
    });
}

function initWheelCenterInteraction() {
    let wheelRafId = null;

    document.querySelectorAll('.wheel-center').forEach(centerEl => {
        centerEl.addEventListener('mousedown', (e) => {
            if (e.button === 0 || e.button === 2) {
                isDraggingWheelCenter = true;
                wheelDragMoved = false;
                wheelDragButton = e.button;
                wheelDragStartX = e.clientX;
                wheelDragStartY = e.clientY;
                wheelStartOffsetX = (currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0;
                wheelStartOffsetY = (currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0;
                centerEl.classList.add('is-dragging');
                const wheelContainer = document.getElementById('wheel-container');
                if (wheelContainer) {
                    wheelContainer.style.transition = 'none';
                    wheelContainer.style.willChange = 'transform';
                }
                e.preventDefault();
                e.stopPropagation();
            }
        });

        centerEl.addEventListener('wheel', (e) => {
            if (isDraggingWheelCenter && wheelDragButton === 0) {
                e.preventDefault();
                e.stopPropagation();
                let delta = e.deltaY < 0 ? 0.05 : -0.05;
                let currentScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
                let newScale = Math.max(0.2, Math.min(3.0, currentScale + delta));
                currentWheelData.wheelScale = newScale;
                tempScale = newScale;

                saveWheelPositionToStorage();
                if (typeof updateWheelUI === 'function') updateWheelUI();
                persistWheelSettings();
            }
        }, { passive: false });
    });

    window.addEventListener('mousemove', (e) => {
        if (!isDraggingWheelCenter) return;

        const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
        const dx = (e.clientX - wheelDragStartX) / scale;
        const dy = (e.clientY - wheelDragStartY) / scale;
        if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
            wheelDragMoved = true;
        }
        if (!wheelDragMoved) return;

        const newOffsetX = wheelStartOffsetX + dx;
        const newOffsetY = wheelStartOffsetY + dy;

        currentWheelData.wheelOffsetX = newOffsetX;
        currentWheelData.wheelOffsetY = newOffsetY;
        tempOffsetX = newOffsetX;
        tempOffsetY = newOffsetY;

        if (!wheelRafId) {
            wheelRafId = requestAnimationFrame(() => {
                wheelRafId = null;
                const container = document.getElementById('wheel-container');
                if (container) {
                    const isEdit = isEditModeActive();
                    const isClassic = typeof isClassicEditMode === 'function' && isClassicEditMode();
                    const baseScale = (isEdit && !isClassic) ? 0.65 : 1.0;
                    const userScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
                    container.style.transform = `translate3d(${newOffsetX}px, ${newOffsetY}px, 0) scale(${baseScale * userScale})`;
                }
            });
        }
    }, { passive: true });

    window.addEventListener('mouseup', (e) => {
        if (isDraggingWheelCenter) {
            isDraggingWheelCenter = false;
            if (wheelRafId) {
                cancelAnimationFrame(wheelRafId);
                wheelRafId = null;
            }
            document.querySelectorAll('.wheel-center').forEach(el => el.classList.remove('is-dragging'));
            const wheelContainer = document.getElementById('wheel-container');
            if (wheelContainer) {
                wheelContainer.style.transition = '';
                wheelContainer.style.willChange = 'auto';
            }
            if (wheelDragMoved) {
                saveWheelPositionToStorage();
                persistWheelSettings();
            } else if (wheelDragButton === 2) {
                const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (window.innerWidth / 2);
                const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (window.innerHeight / 2);
                if (typeof showWheelContextMenu === 'function') showWheelContextMenu(clickX, clickY);
            }
        }
    });
}

function persistWheelSettings() {
    saveWheelPositionToStorage();
    if (typeof window.SaveSettings === 'function' && currentWheelData) {
        window.SaveSettings(JSON.stringify({
            editMode: isEditModeActive(),
            wheelScale: currentWheelData.wheelScale || 1.0,
            wheelOffsetX: currentWheelData.wheelOffsetX || 0.0,
            wheelOffsetY: currentWheelData.wheelOffsetY || 0.0,
            uiScale: currentWheelData.uiScale || 1.0,
            editUiMode: (currentWheelData.editUiMode !== undefined ? currentWheelData.editUiMode : (parseInt(localStorage.getItem('outfit_wheeler_edit_ui_mode')) || 0)),
            editKey: (currentWheelData.editKey !== undefined && currentWheelData.editKey !== null) ? currentWheelData.editKey : 0x31,
            editModifierKey: (currentWheelData.editModifierKey !== undefined && currentWheelData.editModifierKey !== null) ? (parseInt(currentWheelData.editModifierKey, 10) || 0) : 0,
            playKey: (currentWheelData.playKey !== undefined && currentWheelData.playKey !== null) ? currentWheelData.playKey : 0x2F,
            playModifierKey: (currentWheelData.playModifierKey !== undefined && currentWheelData.playModifierKey !== null) ? (parseInt(currentWheelData.playModifierKey, 10) || 0) : 0,
            gamepadPlayKey: (currentWheelData.gamepadPlayKey !== undefined && currentWheelData.gamepadPlayKey !== null) ? (parseInt(currentWheelData.gamepadPlayKey, 10) || 0x0008) : 0x0008,
            gamepadPlayModifier: (currentWheelData.gamepadPlayModifier !== undefined && currentWheelData.gamepadPlayModifier !== null) ? (parseInt(currentWheelData.gamepadPlayModifier, 10) || 0x0100) : 0x0100,
            autoCloseOnEquip: currentWheelData.autoCloseOnEquip !== false,
            autoStashEnabled: currentWheelData.autoStashEnabled === true,
            autoSystemEnabled: currentWheelData.autoSystemEnabled === true,
            autoCityEnabled: currentWheelData.autoCityEnabled !== false,
            autoHomeEnabled: currentWheelData.autoHomeEnabled !== false,
            autoCombatEnabled: currentWheelData.autoCombatEnabled !== false,
            autoSwimEnabled: currentWheelData.autoSwimEnabled !== false
        }));
    }
}

function saveOutfit(slotIndex) {
    if (typeof showSaveToast === 'function') showSaveToast("Outfit Saved!");
    if (typeof window.SaveOutfit === 'function' && currentWheelData) {
        window.SaveOutfit(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: slotIndex
        }));
    } else if (typeof window.SaveOutfitToSlot === 'function' && currentWheelData) {
        window.SaveOutfitToSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: slotIndex
        }));
    }
}

function equipOutfit(slotIndex, keepOpen) {
    const now = Date.now();
    if (now - lastEquipCallTimestamp < 350) return;
    lastEquipCallTimestamp = now;

    const shiftHeld = (typeof isShiftKeyDown !== 'undefined' && isShiftKeyDown) || (window.event && window.event.shiftKey) || false;
    const autoCloseDisabled = (currentWheelData && currentWheelData.autoCloseOnEquip === false);
    const finalKeepOpen = (keepOpen === true) || shiftHeld || autoCloseDisabled || isEditModeActive();

    lastEquippedSlotIndex = slotIndex;
    if (typeof updateWheelUI === 'function') updateWheelUI();
    if (typeof window.EquipSlot === 'function' && currentWheelData) {
        window.EquipSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: slotIndex,
            editMode: isEditModeActive(),
            keepOpen: finalKeepOpen === true
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
    activeWheelContextFilter = 'ALL';
    const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === 'ALL') tab.classList.add('active');
        else tab.classList.remove('active');
    });
    if (typeof window.AddWheel === 'function' && currentWheelData) {
        window.AddWheel(JSON.stringify({
            editMode: isEditModeActive()
        }));
    }
}

function addSlot() {
    if (currentWheelData) {
        if (!Array.isArray(currentWheelData.slots)) {
            currentWheelData.slots = [];
        }
        currentWheelData.editMode = true;
        const newIndex = currentWheelData.slots.length;
        const newTag = (activeWheelContextFilter && activeWheelContextFilter !== 'ALL') ? activeWheelContextFilter : "";
        const newSlot = {
            index: newIndex,
            name: "Slot " + (newIndex + 1),
            hasOutfit: false,
            isInWardrobe: false,
            items: [],
            conditions: [],
            contextTag: newTag
        };
        currentWheelData.slots.push(newSlot);
        if (selectedSlotIndex === null || selectedSlotIndex === undefined || selectedSlotIndex < 0) {
            selectedSlotIndex = newIndex;
        }

        hideAllMenus();
        if (typeof applyModeLayout === 'function') applyModeLayout();
        if (typeof updateWheelUI === 'function') updateWheelUI();
        if (typeof renderSlotsRail === 'function') renderSlotsRail();
        if (typeof renderDrawerDetails === 'function') renderDrawerDetails();

        if (typeof window.AddSlot === 'function') {
            window.AddSlot(JSON.stringify({
                editMode: true,
                contextTag: activeWheelContextFilter && activeWheelContextFilter !== 'ALL' ? activeWheelContextFilter : ""
            }));
        }
        showSaveToast("Slot " + (newIndex + 1) + " Added");
    }
}

function closeWheel() {
    const wrapper = document.getElementById('ui-wrapper');
    if (wrapper) {
        wrapper.classList.remove('is-open');
        wrapper.style.display = 'none';
        wrapper.style.pointerEvents = 'none';
    }
    const poseSelect = document.getElementById('studio-pose-select');
    if (poseSelect) poseSelect.value = 'IdleForceDefaultState';
    activeStudioPose = 'IdleForceDefaultState';
    hideAllMenus();
    if (typeof hideAllModals === 'function') hideAllModals();
    if (typeof window.OnCloseWheel === 'function') {
        window.OnCloseWheel("");
    }
}
window.closeWheel = closeWheel;

function setWheelContextFilter(tag, e) {
    if (e) e.stopPropagation();
    activeWheelContextFilter = tag;

    const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === tag) tab.classList.add('active');
        else tab.classList.remove('active');
    });

    if (typeof updateWheelUI === 'function') updateWheelUI();
}

function setCategoryFilter(cat, e) {
    if (e) e.stopPropagation();
    activeCategoryFilter = cat;

    const chips = document.querySelectorAll('.filter-chips-bar .filter-chip');
    chips.forEach(c => {
        if (c.getAttribute('data-cat') === cat) c.classList.add('active');
        else c.classList.remove('active');
    });

    renderInventoryList();
}

function savePanelsLayout() {
    if (!isEditModeActive() || isClassicEditMode()) return;
    const panelIds = ['panel-inventory', 'right-dock-container'];
    const layout = {};
    panelIds.forEach(id => {
        const el = document.getElementById(id);
        if (el && el.offsetWidth > 100 && el.offsetHeight > 100) {
            const rect = el.getBoundingClientRect();
            if (rect.width > 100 && rect.height > 100) {
                const w = Math.round(rect.width);
                const h = Math.round(rect.height);
                const l = Math.round(rect.left);
                const t = Math.round(rect.top);

                if (id === 'right-dock-container') {
                    if (w < 440 || l < 340) return;
                }
                if (id === 'panel-inventory') {
                    if (w < 260) return;
                }

                layout[id] = {
                    left: l,
                    top: t,
                    width: w,
                    height: h
                };
            }
        }
    });
    if (Object.keys(layout).length > 0) {
        try {
            localStorage.setItem('outfit_wheeler_panel_layout_v5', JSON.stringify(layout));
        } catch (err) {}
    }
}

function loadPanelsLayout() {
    try {
        localStorage.removeItem('outfit_wheeler_panel_layout');
        localStorage.removeItem('outfit_wheeler_panel_layout_v2');
        localStorage.removeItem('outfit_wheeler_panel_layout_v3');
        localStorage.removeItem('outfit_wheeler_panel_layout_v4');
        const raw = localStorage.getItem('outfit_wheeler_panel_layout_v5');
        const screenW = window.innerWidth || 1920;
        const screenH = window.innerHeight || 1080;

        const panelIds = ['panel-inventory', 'right-dock-container'];
        panelIds.forEach(id => {
            const el = document.getElementById(id);
            if (!el) return;
            el.style.left = '';
            el.style.top = '';
            el.style.right = '';
            el.style.bottom = '';
            el.style.width = '';
            el.style.height = '';
            el.style.position = '';
            el.style.transform = '';
            el.style.margin = '';
            el.style.maxWidth = '';
            el.style.maxHeight = '';
        });

        if (raw) {
            const layout = JSON.parse(raw);
            Object.keys(layout).forEach(id => {
                if (id === 'panel-settings') return;
                const data = layout[id];
                const el = document.getElementById(id);
                if (el && data && isFinite(data.left) && isFinite(data.top) && isFinite(data.width) && isFinite(data.height)) {
                    if (id === 'right-dock-container') {
                        if (data.width < 440 || data.left < 340 || (data.left + 440 > screenW + 50)) {
                            el.style.position = 'fixed';
                            el.style.right = '16px';
                            el.style.left = 'auto';
                            el.style.top = '16px';
                            el.style.bottom = '16px';
                            el.style.width = '480px';
                            el.style.height = 'calc(100% - 32px)';
                            return;
                        }
                    }
                    if (id === 'panel-inventory') {
                        if (data.width < 260 || data.left > (screenW - 480)) {
                            el.style.position = 'fixed';
                            el.style.left = '16px';
                            el.style.right = 'auto';
                            el.style.top = '16px';
                            el.style.bottom = '16px';
                            el.style.width = '320px';
                            el.style.height = 'calc(100% - 32px)';
                            return;
                        }
                    }

                    const minW = (id === 'panel-inventory') ? 260 : 440;
                    let w = Math.max(minW, Math.min(data.width, screenW - 20));
                    let h = Math.max(140, Math.min(data.height, screenH - 20));
                    let l = Math.max(10, Math.min(data.left, screenW - w - 10));
                    let t = Math.max(10, Math.min(data.top, screenH - 60));

                    el.style.position = 'fixed';
                    el.style.left = l + 'px';
                    el.style.top = t + 'px';
                    el.style.width = w + 'px';
                    el.style.height = h + 'px';
                    el.style.right = 'auto';
                    el.style.bottom = 'auto';
                }
            });
        }
    } catch (err) {}
}

function resetPanelsLayout() {
    try {
        localStorage.removeItem('outfit_wheeler_panel_layout');
        localStorage.removeItem('outfit_wheeler_panel_layout_v2');
        localStorage.removeItem('outfit_wheeler_panel_layout_v3');
        localStorage.removeItem('outfit_wheeler_panel_layout_v4');
        localStorage.removeItem('outfit_wheeler_panel_layout_v5');
        localStorage.removeItem('outfit_wheeler_rail_width');
    } catch (err) {}

    const inv = document.getElementById('panel-inventory');
    if (inv) {
        inv.style.position = 'fixed';
        inv.style.left = '16px';
        inv.style.right = 'auto';
        inv.style.top = '16px';
        inv.style.bottom = '16px';
        inv.style.width = '320px';
        inv.style.height = 'calc(100% - 32px)';
        inv.style.transform = 'none';
        inv.style.margin = '0';
    }

    const dock = document.getElementById('right-dock-container');
    if (dock) {
        dock.style.position = 'fixed';
        dock.style.right = '16px';
        dock.style.left = 'auto';
        dock.style.top = '16px';
        dock.style.bottom = '16px';
        dock.style.width = '480px';
        dock.style.height = 'calc(100% - 32px)';
        dock.style.transform = 'none';
        dock.style.margin = '0';
    }

    const rail = document.getElementById('panel-slots-rail');
    if (rail) {
        rail.style.width = '120px';
    }

    if (typeof showActionToast === 'function') {
        showActionToast('Layout reset to default');
    }
}

function initCarouselScrollHandlers() {
    const fContainer = document.getElementById('follower-carousel-container');
    if (fContainer) {
        fContainer.addEventListener('wheel', (e) => {
            let rawDelta = (typeof e.deltaY === 'number' && e.deltaY !== 0) ? e.deltaY : (e.wheelDelta ? -e.wheelDelta : (e.detail ? e.detail * 40 : 0));
            if (!rawDelta) return;
            e.preventDefault();
            e.stopPropagation();
            const now = Date.now();
            if (now - lastFollowerScrollTimestamp < 140) return;
            lastFollowerScrollTimestamp = now;
            cycleFollowerCarousel(rawDelta < 0 ? -1 : 1);
        }, { passive: false });
    }

    const srcWrapper = document.getElementById('inv-source-carousel-wrapper');
    if (srcWrapper) {
        srcWrapper.addEventListener('wheel', (e) => {
            let rawDelta = (typeof e.deltaY === 'number' && e.deltaY !== 0) ? e.deltaY : (e.wheelDelta ? -e.wheelDelta : (e.detail ? e.detail * 40 : 0));
            if (!rawDelta) return;
            e.preventDefault();
            e.stopPropagation();
            const now = Date.now();
            if (now - lastSourceScrollTimestamp < 140) return;
            lastSourceScrollTimestamp = now;
            cycleInvSource(rawDelta < 0 ? -1 : 1);
        }, { passive: false });
    }

    const sortList = document.getElementById('custom-sort-list');
    if (sortList) {
        sortList.addEventListener('wheel', (e) => {
            let rawDelta = (typeof e.deltaY === 'number' && e.deltaY !== 0) ? e.deltaY : (e.wheelDelta ? -e.wheelDelta : (e.detail ? e.detail * 40 : 0));
            if (!rawDelta) return;
            e.preventDefault();
            e.stopPropagation();
            const sign = rawDelta > 0 ? 1 : -1;
            sortList.scrollTop += sign * Math.max(40, Math.min(100, Math.abs(rawDelta)));
        }, { passive: false });
    }
}

function initCameraOrbitAndControls() {
    window.addEventListener('mousedown', (e) => {
        if (!isEditModeActive()) return;
        if (typeof isClassicEditMode === 'function' && isClassicEditMode()) return;
        if (e.button !== 0) return;
        if (safeClosest(e.target, '.glass-panel') || safeClosest(e.target, '.fullscreen-modal-backdrop') || safeClosest(e.target, '#top-follower-bar') || safeClosest(e.target, '#studio-pose-bar') || safeClosest(e.target, '.context-menu-popup')) return;

        isOrbitingCamera = true;
        lastCamMouseX = e.clientX;
        lastCamMouseY = e.clientY;
        const viewport = document.getElementById('panel-viewport');
        if (viewport) viewport.classList.add('active');
    });

    let orbitPendingRaf = false;
    let accumulatedOrbitDx = 0;
    let accumulatedOrbitDy = 0;

    window.addEventListener('mousemove', (e) => {
        if (!isOrbitingCamera) return;
        const dx = e.clientX - lastCamMouseX;
        const dy = e.clientY - lastCamMouseY;
        lastCamMouseX = e.clientX;
        lastCamMouseY = e.clientY;

        accumulatedOrbitDx += dx;
        accumulatedOrbitDy += dy;

        if (!orbitPendingRaf) {
            orbitPendingRaf = true;
            requestAnimationFrame(() => {
                orbitPendingRaf = false;
                if (typeof window.OrbitCamera === 'function' && (accumulatedOrbitDx !== 0 || accumulatedOrbitDy !== 0)) {
                    window.OrbitCamera(JSON.stringify({ deltaX: accumulatedOrbitDx, deltaY: accumulatedOrbitDy }));
                    accumulatedOrbitDx = 0;
                    accumulatedOrbitDy = 0;
                }
            });
        }
    });

    window.addEventListener('mouseup', (e) => {
        if (e.button === 0 && isOrbitingCamera) {
            isOrbitingCamera = false;
            accumulatedOrbitDx = 0;
            accumulatedOrbitDy = 0;
            const viewport = document.getElementById('panel-viewport');
            if (viewport) viewport.classList.remove('active');
        }
    });

    window.addEventListener('keydown', (e) => {
        if (!isEditModeActive()) return;
        if (typeof isClassicEditMode === 'function' && isClassicEditMode()) return;
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT' || e.target.tagName === 'TEXTAREA') return;

        const key = e.key.toLowerCase();
        let moveX = 0;
        let moveY = 0;

        if (key === 'a') moveX = -1;
        else if (key === 'd') moveX = 1;
        else if (key === 'w') moveY = 1;
        else if (key === 's') moveY = -1;

        if ((moveX !== 0 || moveY !== 0) && typeof window.PanCamera === 'function') {
            window.PanCamera(JSON.stringify({ moveX: moveX, moveY: moveY }));
        }
    });
}

var activeStudioPose = 'IdleForceDefaultState';

function onStudioPoseChange(poseEvent) {
    activeStudioPose = poseEvent || 'IdleForceDefaultState';
    const sel = document.getElementById('studio-pose-select');
    if (sel) {
        sel.value = activeStudioPose;
        if (sel.options && typeof sel.options.length === 'number') {
            let optExists = false;
            for (let i = 0; i < sel.options.length; i++) {
                if (sel.options[i] && sel.options[i].value === activeStudioPose) {
                    optExists = true;
                    break;
                }
            }
            if (!optExists && typeof document.createElement === 'function') {
                const opt = document.createElement('option');
                opt.value = activeStudioPose;
                opt.text = activeStudioPose;
                sel.appendChild(opt);
            }
        }
    }
    const menuItems = (typeof document.querySelectorAll === 'function') ? document.querySelectorAll('.pose-menu-item') : null;
    let matchedName = null;
    if (menuItems && menuItems.forEach) {
        menuItems.forEach(mi => {
            const isMatch = (mi.getAttribute('data-pose') === activeStudioPose);
            mi.classList.toggle('active', isMatch);
            const chk = mi.querySelector('.pose-check');
            if (chk) chk.style.display = isMatch ? 'inline-block' : 'none';
            if (isMatch) {
                matchedName = mi.getAttribute('data-name') || mi.innerText.replace('✓', '').trim();
            }
        });
    }
    const label = document.getElementById('pose-selected-label');
    if (label) {
        if (matchedName) {
            label.innerText = matchedName;
        } else if (sel && sel.options && sel.selectedIndex >= 0) {
            label.innerText = sel.options[sel.selectedIndex].text;
        } else {
            label.innerText = activeStudioPose;
        }
    }
    const actorId = activeInventorySource || (currentWheelData && currentWheelData.activeTargetActorId) || '0x00000007';
    if (typeof window.SetStudioPose === 'function') {
        window.SetStudioPose(JSON.stringify({
            actorID: actorId,
            eventName: activeStudioPose
        }));
    }
}

function togglePoseDropdown(e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    const menu = document.getElementById('studio-pose-dropdown-menu');
    const trigger = document.getElementById('btn-pose-dropdown-trigger');
    if (!menu || !trigger) return;
    const isShowing = (menu.style.display === 'flex' || menu.style.display === 'block');
    if (isShowing) {
        menu.style.display = 'none';
        trigger.classList.remove('active');
    } else {
        hideAllMenus();
        if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
        menu.style.display = 'flex';
        trigger.classList.add('active');
        if (!menu._hasWheelListener) {
            menu._hasWheelListener = true;
            menu.addEventListener('wheel', (ev) => {
                let rawDelta = (typeof ev.deltaY === 'number' && ev.deltaY !== 0) ? ev.deltaY : (ev.wheelDelta ? -ev.wheelDelta : (ev.detail ? ev.detail * 40 : 0));
                if (!rawDelta) return;
                ev.preventDefault();
                ev.stopPropagation();
                const sign = rawDelta > 0 ? 1 : -1;
                menu.scrollTop += sign * Math.max(40, Math.min(100, Math.abs(rawDelta)));
            }, { passive: false });
        }
    }
}

function selectStudioPoseOption(poseId, poseName, e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    const menu = document.getElementById('studio-pose-dropdown-menu');
    const trigger = document.getElementById('btn-pose-dropdown-trigger');
    if (menu) menu.style.display = 'none';
    if (trigger) trigger.classList.remove('active');
    onStudioPoseChange(poseId);
}

function resetStudioPose(e) {
    if (e) {
        if (typeof e.preventDefault === 'function') e.preventDefault();
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
    }
    onStudioPoseChange('IdleForceDefaultState');
}

let lastPopulatedPosePackCount = -1;
function populatePosePacks(packs) {
    if (!Array.isArray(packs) || packs.length === 0) return;
    const select = document.getElementById('studio-pose-select');
    const dropdownMenu = document.getElementById('studio-pose-dropdown-menu');
    if (!select && !dropdownMenu) return;

    if (packs.length === lastPopulatedPosePackCount) {
        if (select) select.value = activeStudioPose || 'IdleForceDefaultState';
        return;
    }
    lastPopulatedPosePackCount = packs.length;

    if (select) {
        select.innerHTML = '<option value="IdleForceDefaultState">Default Stand</option>';
        packs.forEach(pack => {
            if (!pack || !pack.packName || !Array.isArray(pack.poses) || pack.poses.length === 0) return;
            const optgroup = document.createElement('optgroup');
            optgroup.label = pack.packName;
            pack.poses.forEach(pose => {
                if (!pose || !pose.id) return;
                const opt = document.createElement('option');
                opt.value = pose.id;
                opt.text = pose.name || pose.id;
                optgroup.appendChild(opt);
            });
            select.appendChild(optgroup);
        });
        select.value = activeStudioPose || 'IdleForceDefaultState';
    }

    if (dropdownMenu) {
        dropdownMenu.innerHTML = '';
        const defItem = document.createElement('div');
        defItem.className = 'pose-menu-item' + (activeStudioPose === 'IdleForceDefaultState' ? ' active' : '');
        defItem.setAttribute('data-pose', 'IdleForceDefaultState');
        defItem.setAttribute('data-name', 'Default Stand');
        defItem.onmousedown = (e) => selectStudioPoseOption('IdleForceDefaultState', 'Default Stand', e);
        defItem.onclick = (e) => selectStudioPoseOption('IdleForceDefaultState', 'Default Stand', e);
        defItem.innerHTML = '<span>Default Stand</span><span class="pose-check" style="' + (activeStudioPose === 'IdleForceDefaultState' ? 'display:inline-block;' : 'display:none;') + '">&#x2713;</span>';
        dropdownMenu.appendChild(defItem);

        packs.forEach(pack => {
            if (!pack || !pack.packName || !Array.isArray(pack.poses) || pack.poses.length === 0) return;
            const groupLabel = document.createElement('div');
            groupLabel.className = 'pose-menu-group-label';
            groupLabel.innerText = pack.packName;
            dropdownMenu.appendChild(groupLabel);

            pack.poses.forEach(pose => {
                if (!pose || !pose.id) return;
                const pId = pose.id;
                const pName = pose.name || pId;
                const isMatch = (activeStudioPose === pId);
                const item = document.createElement('div');
                item.className = 'pose-menu-item' + (isMatch ? ' active' : '');
                item.setAttribute('data-pose', pId);
                item.setAttribute('data-name', pName);
                item.onmousedown = (e) => selectStudioPoseOption(pId, pName, e);
                item.onclick = (e) => selectStudioPoseOption(pId, pName, e);

                const nameSpan = document.createElement('span');
                nameSpan.innerText = pName;
                const checkSpan = document.createElement('span');
                checkSpan.className = 'pose-check';
                checkSpan.innerHTML = '&#x2713;';
                checkSpan.style.display = isMatch ? 'inline-block' : 'none';

                item.appendChild(nameSpan);
                item.appendChild(checkSpan);
                dropdownMenu.appendChild(item);
            });
        });
    }

    const currentLabel = document.getElementById('pose-selected-label');
    if (currentLabel) {
        const item = (typeof document.querySelector === 'function') ? document.querySelector(`.pose-menu-item[data-pose="${activeStudioPose}"]`) : null;
        if (item) {
            currentLabel.innerText = item.getAttribute('data-name') || item.innerText.replace('✓', '').trim();
        } else if (activeStudioPose === 'IdleForceDefaultState') {
            currentLabel.innerText = 'Default Stand';
        }
    }
}

window.populatePosePacks = populatePosePacks;
window.onPosePacksReady = function(jsonPayload) {
    try {
        const packs = (typeof jsonPayload === 'string') ? JSON.parse(jsonPayload) : jsonPayload;
        if (Array.isArray(packs)) {
            populatePosePacks(packs);
        }
    } catch (e) {}
};

window.onStudioPoseChange = onStudioPoseChange;
window.resetStudioPose = resetStudioPose;
window.togglePoseDropdown = togglePoseDropdown;
window.selectStudioPoseOption = selectStudioPoseOption;

function initDraggablePanels() {
    panelRafId = null;

    const targets = document.querySelectorAll('.draggable-panel');
    targets.forEach(panel => {
        if (panel.id === 'panel-settings') return;
        panel.addEventListener('mousedown', (e) => {
            const isHeader = !!safeClosest(e.target, '.panel-drag-handle');
            if (isHeader && (e.button === 0 || e.button === 2)) {
                e.preventDefault();
                startPanelDrag(e, panel.id);
            }
        });
        panel.addEventListener('contextmenu', (e) => {
            e.preventDefault();
        });
    });

    window.addEventListener('mousemove', (e) => {
        if (!activeDragPanelId || activeDragPanelId === 'panel-settings') return;

        const currentClientX = e.clientX;
        const currentClientY = e.clientY;

        if (!panelRafId) {
            panelRafId = requestAnimationFrame(() => {
                panelRafId = null;
                const panel = document.getElementById(activeDragPanelId);
                if (!panel) return;

                const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
                const screenW = (window.innerWidth || 1920) / scale;
                const screenH = (window.innerHeight || 1080) / scale;
                const panelW = panel.offsetWidth;
                const panelH = panel.offsetHeight;

                let rawLeft = panelStartLeft + ((currentClientX - dragStartX) / scale);
                let rawTop = panelStartTop + ((currentClientY - dragStartY) / scale);

                const SNAP_THRESHOLD = 18;
                const SCREEN_MARGIN = 14;
                const PANEL_GAP = 12;

                let snappedLeft = rawLeft;
                let snappedTop = rawTop;

                if (Math.abs(rawLeft - SCREEN_MARGIN) < SNAP_THRESHOLD) {
                    snappedLeft = SCREEN_MARGIN;
                } else if (Math.abs(rawLeft - (screenW - panelW - SCREEN_MARGIN)) < SNAP_THRESHOLD) {
                    snappedLeft = screenW - panelW - SCREEN_MARGIN;
                }

                if (Math.abs(rawTop - SCREEN_MARGIN) < SNAP_THRESHOLD) {
                    snappedTop = SCREEN_MARGIN;
                } else if (Math.abs(rawTop - (screenH - panelH - SCREEN_MARGIN)) < SNAP_THRESHOLD) {
                    snappedTop = screenH - panelH - SCREEN_MARGIN;
                }

                document.querySelectorAll('.draggable-panel').forEach(other => {
                    if (other.id === activeDragPanelId || other.id === 'panel-settings' || other.style.display === 'none') return;
                    const oRect = other.getBoundingClientRect();
                    const oLeft = oRect.left;
                    const oRight = oRect.right;
                    const oTop = oRect.top;
                    const oBottom = oRect.bottom;

                    if (Math.abs(snappedLeft - oLeft) < SNAP_THRESHOLD) {
                        snappedLeft = oLeft;
                    } else if (Math.abs(snappedLeft - (oRight + PANEL_GAP)) < SNAP_THRESHOLD) {
                        snappedLeft = oRight + PANEL_GAP;
                    } else if (Math.abs((snappedLeft + panelW) - oLeft) < SNAP_THRESHOLD) {
                        snappedLeft = oLeft - panelW - PANEL_GAP;
                    } else if (Math.abs((snappedLeft + panelW) - oRight) < SNAP_THRESHOLD) {
                        snappedLeft = oRight - panelW;
                    }

                    if (Math.abs(snappedTop - oTop) < SNAP_THRESHOLD) {
                        snappedTop = oTop;
                    } else if (Math.abs(snappedTop - (oBottom + PANEL_GAP)) < SNAP_THRESHOLD) {
                        snappedTop = oBottom + PANEL_GAP;
                    } else if (Math.abs((snappedTop + panelH) - oTop) < SNAP_THRESHOLD) {
                        snappedTop = oTop - panelH - PANEL_GAP;
                    } else if (Math.abs((snappedTop + panelH) - oBottom) < SNAP_THRESHOLD) {
                        snappedTop = oBottom - panelH;
                    }
                });

                snappedLeft = Math.max(SCREEN_MARGIN, Math.min(snappedLeft, screenW - panelW - SCREEN_MARGIN));
                snappedTop = Math.max(SCREEN_MARGIN, Math.min(snappedTop, screenH - panelH - SCREEN_MARGIN));

                panel.style.position = 'fixed';
                panel.style.left = Math.round(snappedLeft) + 'px';
                panel.style.top = Math.round(snappedTop) + 'px';
                panel.style.right = 'auto';
                panel.style.bottom = 'auto';
            });
        }
    }, { passive: true });

    window.addEventListener('mouseup', endPanelDrag, true);
    window.addEventListener('pointerup', endPanelDrag, true);
    window.addEventListener('pointercancel', endPanelDrag, true);
    window.addEventListener('blur', endPanelDrag, true);
}

function endPanelDrag() {
    if (activeDragPanelId) {
        if (panelRafId) {
            cancelAnimationFrame(panelRafId);
            panelRafId = null;
        }
        const panel = document.getElementById(activeDragPanelId);
        if (panel) panel.classList.remove('is-dragging');
        try {
            savePanelsLayout();
        } catch (err) {}
        activeDragPanelId = null;
    }
    document.querySelectorAll('.is-dragging').forEach(el => el.classList.remove('is-dragging'));
}

function handlePanelDragStart(e, panelId) {
    if (e.button === 0 || e.button === 2) {
        e.preventDefault();
        e.stopPropagation();
        startPanelDrag(e, panelId);
    }
}
function startPanelDrag(e, panelId) {
    activeDragPanelId = panelId;
    dragStartX = e.clientX;
    dragStartY = e.clientY;
    const panel = document.getElementById(panelId);
    if (panel) {
        panel.classList.add('is-dragging');
        const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
        const rect = panel.getBoundingClientRect();
        panelStartLeft = (typeof panel.offsetLeft === 'number' && !isNaN(panel.offsetLeft) && panel.offsetLeft > 0) ? panel.offsetLeft : (rect.left / scale);
        panelStartTop = (typeof panel.offsetTop === 'number' && !isNaN(panel.offsetTop) && panel.offsetTop > 0) ? panel.offsetTop : (rect.top / scale);
        panel.style.position = 'fixed';
        panel.style.left = Math.round(panelStartLeft) + 'px';
        panel.style.top = Math.round(panelStartTop) + 'px';
        panel.style.right = 'auto';
        panel.style.bottom = 'auto';
        panel.style.transform = 'none';
        panel.style.margin = '0';
        panel.style.maxWidth = 'none';
        panel.style.maxHeight = 'none';
    }
}

function initResizablePanels() {
    window.addEventListener('pointermove', handlePanelResizeMove, { passive: true });
    window.addEventListener('mousemove', handlePanelResizeMove, { passive: true });

    window.addEventListener('pointerup', endPanelResize, true);
    window.addEventListener('mouseup', endPanelResize, true);
    window.addEventListener('pointercancel', endPanelResize, true);
    window.addEventListener('blur', endPanelResize, true);
}

function handlePanelResizeMove(e) {
    if (!activeResizePanelId || activeResizePanelId === 'panel-settings') return;
    resizeCurrentClientX = e.clientX;
    resizeCurrentClientY = e.clientY;

    if (!resizeRafPending) {
        resizeRafPending = true;
        requestAnimationFrame(() => {
            resizeRafPending = false;
            if (!activeResizePanelId || activeResizePanelId === 'panel-settings') return;
            const panel = document.getElementById(activeResizePanelId);
            if (!panel) return;

            const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
            const dx = (resizeCurrentClientX - resizeStartX) / scale;
            const dy = (resizeCurrentClientY - resizeStartY) / scale;
            const screenW = (window.innerWidth || 1920) / scale;
            const screenH = (window.innerHeight || 1080) / scale;

            let newW = panelStartWidth;
            let newH = panelStartHeight;
            let newLeft = panelStartLeft;
            let newTop = panelStartTop;

            const SNAP_THRESHOLD = 18;
            const SCREEN_MARGIN = 14;
            const minWidthForPanel = (activeResizePanelId === 'right-dock-container') ? 440 : 260;

            if (resizeDirection.includes('r')) {
                newW = Math.max(minWidthForPanel, Math.min(screenW - panelStartLeft - SCREEN_MARGIN, panelStartWidth + dx));
                if (Math.abs((newLeft + newW) - (screenW - SCREEN_MARGIN)) < SNAP_THRESHOLD) {
                    newW = screenW - SCREEN_MARGIN - newLeft;
                }
            }
            if (resizeDirection.includes('l')) {
                let candidateLeft = panelStartLeft + dx;
                if (Math.abs(candidateLeft - SCREEN_MARGIN) < SNAP_THRESHOLD) {
                    candidateLeft = SCREEN_MARGIN;
                }
                newW = Math.max(minWidthForPanel, (panelStartLeft + panelStartWidth) - candidateLeft);
                newLeft = (panelStartLeft + panelStartWidth) - newW;
            }
            if (resizeDirection.includes('b')) {
                newH = Math.max(150, Math.min(screenH - panelStartTop - SCREEN_MARGIN, panelStartHeight + dy));
                if (Math.abs((newTop + newH) - (screenH - SCREEN_MARGIN)) < SNAP_THRESHOLD) {
                    newH = screenH - SCREEN_MARGIN - newTop;
                }
            }
            if (resizeDirection.includes('t')) {
                let candidateTop = panelStartTop + dy;
                if (Math.abs(candidateTop - SCREEN_MARGIN) < SNAP_THRESHOLD) {
                    candidateTop = SCREEN_MARGIN;
                }
                newH = Math.max(150, (panelStartTop + panelStartHeight) - candidateTop);
                newTop = (panelStartTop + panelStartHeight) - newH;
            }

            panel.style.width = Math.round(newW) + 'px';
            panel.style.height = Math.round(newH) + 'px';
            panel.style.left = Math.round(newLeft) + 'px';
            panel.style.top = Math.round(newTop) + 'px';
        });
    }
}

function endPanelResize(e) {
    if (e && e.target && typeof e.target.releasePointerCapture === 'function' && e.pointerId !== undefined) {
        try {
            e.target.releasePointerCapture(e.pointerId);
        } catch (err) {}
    }
    if (activeResizePanelId) {
        const panel = document.getElementById(activeResizePanelId);
        if (panel) panel.classList.remove('is-resizing');
        activeResizePanelId = null;
        resizeDirection = '';
        try {
            savePanelsLayout();
        } catch (err) {}
    }
    document.querySelectorAll('.is-resizing').forEach(el => el.classList.remove('is-resizing'));
    document.body.style.cursor = '';
    document.body.style.userSelect = '';
}

function handlePanelResizeStart(e, panelId, dir) {
    if (e.button !== 0) return;
    if (activeResizePanelId) return;
    e.preventDefault();
    e.stopPropagation();

    const panel = document.getElementById(panelId);
    if (!panel) return;

    if (e.target && typeof e.target.setPointerCapture === 'function' && e.pointerId !== undefined) {
        try {
            e.target.setPointerCapture(e.pointerId);
        } catch (err) {}
    }

    document.querySelectorAll('.is-resizing').forEach(el => el.classList.remove('is-resizing'));
    panel.classList.add('is-resizing');

    panelStartWidth = panel.offsetWidth;
    panelStartHeight = panel.offsetHeight;
    panelStartLeft = panel.offsetLeft;
    panelStartTop = panel.offsetTop;

    panel.style.position = 'fixed';
    panel.style.left = panelStartLeft + 'px';
    panel.style.top = panelStartTop + 'px';
    panel.style.width = panelStartWidth + 'px';
    panel.style.height = panelStartHeight + 'px';
    panel.style.right = 'auto';
    panel.style.bottom = 'auto';
    panel.style.maxWidth = 'none';
    panel.style.maxHeight = 'none';
    panel.style.transform = 'none';
    panel.style.margin = '0';

    activeResizePanelId = panelId;
    resizeDirection = dir;
    resizeStartX = e.clientX;
    resizeStartY = e.clientY;
    resizeCurrentClientX = e.clientX;
    resizeCurrentClientY = e.clientY;

    if (dir === 't' || dir === 'b') document.body.style.cursor = 'ns-resize';
    else if (dir === 'l' || dir === 'r') document.body.style.cursor = 'ew-resize';
    else if (dir === 'tl' || dir === 'br') document.body.style.cursor = 'nwse-resize';
    else if (dir === 'tr' || dir === 'bl') document.body.style.cursor = 'nesw-resize';
    document.body.style.userSelect = 'none';
}

function initSlotsRailResizer() {
    const splitter = document.getElementById('dock-rail-splitter');
    const rail = document.getElementById('panel-slots-rail');
    if (!splitter || !rail) return;

    let isResizingRail = false;
    let railStartX = 0;
    let railStartWidth = 120;
    let railRafId = null;

    const savedWidth = parseInt(localStorage.getItem('outfit_wheeler_rail_width'), 10);
    if (!isNaN(savedWidth) && savedWidth >= 85 && savedWidth <= 220) {
        rail.style.width = savedWidth + 'px';
    }

    splitter.addEventListener('mousedown', (e) => {
        if (e.button !== 0) return;
        e.preventDefault();
        e.stopPropagation();

        isResizingRail = true;
        railStartX = e.clientX;
        railStartWidth = rail.offsetWidth;
        splitter.classList.add('is-dragging');
        document.body.style.cursor = 'col-resize';
        document.body.style.userSelect = 'none';
    });

    splitter.addEventListener('dblclick', (e) => {
        e.preventDefault();
        e.stopPropagation();
        rail.style.width = '120px';
        localStorage.setItem('outfit_wheeler_rail_width', '120');
    });

    window.addEventListener('mousemove', (e) => {
        if (!isResizingRail) return;
        const currentClientX = e.clientX;

        if (!railRafId) {
            railRafId = requestAnimationFrame(() => {
                railRafId = null;
                if (!isResizingRail) return;
                const scale = (typeof getUiScale === 'function') ? (getUiScale() || 1.0) : 1.0;
                const dx = (currentClientX - railStartX) / scale;
                const newWidth = Math.max(85, Math.min(220, railStartWidth + dx));
                rail.style.width = newWidth + 'px';
            });
        }
    });

    window.addEventListener('mouseup', () => {
        if (isResizingRail) {
            isResizingRail = false;
            if (railRafId) {
                cancelAnimationFrame(railRafId);
                railRafId = null;
            }
            splitter.classList.remove('is-dragging');
            document.body.style.cursor = '';
            document.body.style.userSelect = '';
            const finalWidth = rail.offsetWidth;
            localStorage.setItem('outfit_wheeler_rail_width', String(finalWidth));
        }
    });
}

function initCustomItemDragDrop() {
    window.addEventListener('mousemove', (e) => {
        if (activeDraggedItem) {
            const dx = e.clientX - dragItemStartX;
            const dy = e.clientY - dragItemStartY;
            const dist = Math.sqrt(dx * dx + dy * dy);

            if (dist > 4 && !isItemDragging) {
                isItemDragging = true;
                if (!dragGhostElement) {
                    dragGhostElement = document.createElement('div');
                    dragGhostElement.id = 'dragged-item-ghost';
                    dragGhostElement.style.position = 'fixed';
                    dragGhostElement.style.pointerEvents = 'none';
                    dragGhostElement.style.zIndex = '999999';
                    dragGhostElement.innerHTML = '<span class="ghost-type-badge">' + (activeDraggedItem.type || 'Armor') + '</span><span>' + activeDraggedItem.name + '</span>';
                    document.body.appendChild(dragGhostElement);
                }
            }

            if (isItemDragging && dragGhostElement) {
                dragGhostElement.style.left = (e.clientX + 12) + 'px';
                dragGhostElement.style.top = (e.clientY + 12) + 'px';

                document.querySelectorAll('.drag-hover-target').forEach(el => el.classList.remove('drag-hover-target'));

                const elemBelow = getElementUnderPoint(e.clientX, e.clientY);
                if (elemBelow) {
                    const targetSlotCard = safeClosest(elemBelow, '.vertical-slot-card');
                    const targetSlicePath = safeClosest(elemBelow, '.slice-path');
                    const targetLabel = safeClosest(elemBelow, '.wheel-slot-label');
                    const targetInspector = safeClosest(elemBelow, '#panel-slot-details, #right-dock-container, #panel-slots-rail');

                    if (targetSlotCard) targetSlotCard.classList.add('drag-hover-target');
                    else if (targetSlicePath) targetSlicePath.classList.add('drag-hover-target');
                    else if (targetLabel) targetLabel.classList.add('drag-hover-target');
                    else if (targetInspector) targetInspector.classList.add('drag-hover-target');
                }
            }
        }

        if (activeDraggedSlotIndex !== null) {
            const dx = e.clientX - slotDragStartX;
            const dy = e.clientY - slotDragStartY;
            const dist = Math.sqrt(dx * dx + dy * dy);

            if (dist > 4 && !isSlotDragging) {
                isSlotDragging = true;
                if (!slotDragGhostElement) {
                    slotDragGhostElement = document.createElement('div');
                    slotDragGhostElement.id = 'dragged-slot-ghost';
                    slotDragGhostElement.style.position = 'fixed';
                    slotDragGhostElement.style.pointerEvents = 'none';
                    slotDragGhostElement.style.zIndex = '999999';
                    const slotObj = (currentWheelData.slots || [])[activeDraggedSlotIndex];
                    const slotName = slotObj && slotObj.name ? slotObj.name : ('Slot ' + (activeDraggedSlotIndex + 1));
                    slotDragGhostElement.innerHTML = '<span class="ghost-slot-badge">SLOT ' + (activeDraggedSlotIndex + 1) + '</span><span>' + slotName + '</span>';
                    document.body.appendChild(slotDragGhostElement);
                }
            }

            if (isSlotDragging && slotDragGhostElement) {
                slotDragGhostElement.style.left = (e.clientX + 12) + 'px';
                slotDragGhostElement.style.top = (e.clientY + 12) + 'px';

                document.querySelectorAll('.slot-drop-hover').forEach(el => el.classList.remove('slot-drop-hover'));

                const elemBelow = getElementUnderPoint(e.clientX, e.clientY);
                if (elemBelow) {
                    const targetCard = safeClosest(elemBelow, '.vertical-slot-card');
                    if (targetCard) {
                        targetCard.classList.add('slot-drop-hover');
                    }
                }
            }
        }
    });

    window.addEventListener('mouseup', (e) => {
        if (activeDraggedItem) {
            if (isItemDragging) {
                const elemBelow = getElementUnderPoint(e.clientX, e.clientY);
                if (elemBelow) {
                    const targetSlotCard = safeClosest(elemBelow, '.vertical-slot-card');
                    const targetSlicePath = safeClosest(elemBelow, '.slice-path');
                    const targetLabel = safeClosest(elemBelow, '.wheel-slot-label');
                    const targetInspector = safeClosest(elemBelow, '#panel-slot-details, #right-dock-container, #panel-slots-rail');

                    let targetSlotIdx = selectedSlotIndex;
                    let foundTarget = false;

                    if (targetSlotCard) {
                        const idx = parseInt(targetSlotCard.getAttribute('data-slot-index'), 10);
                        if (!isNaN(idx)) {
                            targetSlotIdx = idx;
                            foundTarget = true;
                        }
                    } else if (targetSlicePath) {
                        const idx = parseInt(targetSlicePath.getAttribute('data-slot-index'), 10);
                        if (!isNaN(idx)) {
                            targetSlotIdx = idx;
                            foundTarget = true;
                        }
                    } else if (targetLabel) {
                        const idx = parseInt(targetLabel.getAttribute('data-slot-index'), 10);
                        if (!isNaN(idx)) {
                            targetSlotIdx = idx;
                            foundTarget = true;
                        }
                    } else if (targetInspector) {
                        foundTarget = true;
                    }

                    if (foundTarget) {
                        const rawIdVal = activeDraggedItem.rawID || activeDraggedItem.formID || parseInt(activeDraggedItem.id, 16) || 0;
                        const idStrVal = activeDraggedItem.id || ('0x' + Number(rawIdVal).toString(16).toUpperCase());
                        const uniqueIdVal = activeDraggedItem.uniqueID || 0;
                        const enchIdVal = activeDraggedItem.enchantmentID || 0;
                        const customNameVal = (activeDraggedItem.customName && activeDraggedItem.customName !== 'Apparel') ? activeDraggedItem.customName : '';

                        if (typeof window.AddItemToSlot === 'function' && currentWheelData) {
                            window.AddItemToSlot(JSON.stringify({
                                wheelIndex: currentWheelData.wheelIndex || 0,
                                slotIndex: targetSlotIdx,
                                rawID: rawIdVal,
                                id: idStrVal,
                                uniqueID: uniqueIdVal,
                                enchantmentID: enchIdVal,
                                customName: customNameVal
                            }));
                        }

                        if (currentWheelData && currentWheelData.slots) {
                            let targetSlot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === targetSlotIdx);
                            if (!targetSlot && currentWheelData.slots[targetSlotIdx]) {
                                targetSlot = currentWheelData.slots[targetSlotIdx];
                            }

                            if (targetSlot) {
                                if (!targetSlot.items) targetSlot.items = [];

                                const alreadyExists = targetSlot.items.some(it => {
                                    const itRaw = (typeof it === 'object') ? (it.rawID || it.formID || parseInt(it.id, 16)) : (typeof it === 'number' ? it : parseInt(it, 16));
                                    const itUID = (typeof it === 'object') ? (it.uniqueID || 0) : 0;
                                    const itEnch = (typeof it === 'object') ? (it.enchantmentID || 0) : 0;
                                    const itName = (typeof it === 'object') ? (it.customName || '') : '';
                                    return itRaw === rawIdVal && itUID === uniqueIdVal && itEnch === enchIdVal && itName === customNameVal;
                                });

                                if (!alreadyExists) {
                                    targetSlot.items.push({
                                        name: activeDraggedItem.name,
                                        rawID: rawIdVal,
                                        id: idStrVal,
                                        uniqueID: uniqueIdVal,
                                        enchantmentID: enchIdVal,
                                        customName: customNameVal,
                                        type: activeDraggedItem.type || 'Armor'
                                    });
                                }

                                targetSlot.hasOutfit = true;
                                targetSlot.count = targetSlot.items.length;
                                if (!targetSlot.name || targetSlot.name === 'Empty Slot') {
                                    targetSlot.name = 'Outfit ' + (targetSlotIdx + 1);
                                }

                                selectedSlotIndex = targetSlotIdx;
                                renderSlotsRail();
                                renderDrawerDetails();
                                if (typeof updateWheelUI === 'function') updateWheelUI();
                            }
                        }

                        if (typeof showSaveToast === 'function') {
                            showSaveToast('Added ' + activeDraggedItem.name + ' to Slot ' + (targetSlotIdx + 1));
                        }
                    }
                }
            }

            document.querySelectorAll('.drag-hover-target').forEach(el => el.classList.remove('drag-hover-target'));
            if (dragGhostElement) {
                dragGhostElement.remove();
                dragGhostElement = null;
            }
            activeDraggedItem = null;
            isItemDragging = false;
        }

        if (activeDraggedSlotIndex !== null) {
            if (isSlotDragging) {
                const elemBelow = getElementUnderPoint(e.clientX, e.clientY);
                if (elemBelow) {
                    const targetCard = safeClosest(elemBelow, '.vertical-slot-card');
                    if (targetCard) {
                        const targetIdx = parseInt(targetCard.getAttribute('data-slot-index'), 10);
                        if (!isNaN(targetIdx) && targetIdx !== activeDraggedSlotIndex && currentWheelData && currentWheelData.slots) {
                            const fromIdx = activeDraggedSlotIndex;
                            const toIdx = targetIdx;

                            if (typeof window.MoveSlot === 'function') {
                                window.MoveSlot(JSON.stringify({
                                    wheelIndex: currentWheelData.wheelIndex || 0,
                                    fromIndex: fromIdx,
                                    toIndex: toIdx,
                                    editMode: isEditModeActive()
                                }));
                            }

                            const movedSlot = currentWheelData.slots.splice(fromIdx, 1)[0];
                            currentWheelData.slots.splice(toIdx, 0, movedSlot);
                            currentWheelData.slots.forEach((s, i) => s.index = i);

                            selectedSlotIndex = toIdx;
                            renderSlotsRail();
                            renderDrawerDetails();
                            if (typeof updateWheelUI === 'function') updateWheelUI();
                            showSaveToast("Slot Moved to Position " + (toIdx + 1));
                        }
                    }
                }
            }

            document.querySelectorAll('.slot-drop-hover').forEach(el => el.classList.remove('slot-drop-hover'));
            if (slotDragGhostElement) {
                slotDragGhostElement.remove();
                slotDragGhostElement = null;
            }
            activeDraggedSlotIndex = null;
            isSlotDragging = false;
        }
    });

    window.addEventListener('blur', () => {
        if (dragGhostElement) {
            dragGhostElement.remove();
            dragGhostElement = null;
        }
        activeDraggedItem = null;
        isItemDragging = false;
        document.querySelectorAll('.drag-hover-target').forEach(el => el.classList.remove('drag-hover-target'));

        if (slotDragGhostElement) {
            slotDragGhostElement.remove();
            slotDragGhostElement = null;
        }
        activeDraggedSlotIndex = null;
        isSlotDragging = false;
        document.querySelectorAll('.slot-drop-hover').forEach(el => el.classList.remove('slot-drop-hover'));

        if (isDraggingWheelCenter) {
            isDraggingWheelCenter = false;
            document.querySelectorAll('.wheel-center').forEach(el => el.classList.remove('is-dragging'));
        }
        if (activeDragPanelId) {
            const panel = document.getElementById(activeDragPanelId);
            if (panel) panel.classList.remove('is-dragging');
            activeDragPanelId = null;
        }
        if (activeResizePanelId) {
            const panel = document.getElementById(activeResizePanelId);
            if (panel) panel.classList.remove('is-resizing');
            activeResizePanelId = null;
        }
        document.querySelectorAll('.is-resizing').forEach(el => el.classList.remove('is-resizing'));
        document.querySelectorAll('.is-dragging').forEach(el => el.classList.remove('is-dragging'));
        const splitter = document.getElementById('dock-rail-splitter');
        if (splitter) splitter.classList.remove('is-dragging');
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
        isOrbitingCamera = false;
        const viewport = document.getElementById('panel-viewport');
        if (viewport) viewport.classList.remove('active');
    });
}

function cycleFollowerCarousel(delta) {
    const list = currentWheelData.followers || [];
    if (list.length === 0) return;

    const activeTarget = (currentWheelData.activeTargetActorId || '0x00000007').toUpperCase();
    let curIdx = list.findIndex(f => (f.id || '').toUpperCase() === activeTarget);
    if (curIdx === -1) curIdx = 0;

    let nextIdx = (curIdx + delta + list.length) % list.length;
    selectFollower(list[nextIdx].id);
}

function renderFollowersStrip() {
    const strip = document.getElementById('follower-strip');
    if (!strip) return;

    const list = currentWheelData.followers || [];
    const activeTarget = (currentWheelData.activeTargetActorId || '0x00000007').toUpperCase();

    if (list.length === 0) {
        strip.innerHTML = '<div class="follower-tab-item active" onclick="selectFollower(\'0x00000007\')"><span class="follower-name">Player Character</span></div>';
        return;
    }

    let activeIdx = list.findIndex(f => (f.id || '').toUpperCase() === activeTarget);
    if (activeIdx === -1) activeIdx = 0;

    let displayList = [];
    const total = list.length;

    if (total <= 3) {
        displayList = list.map((item, originalIndex) => ({ ...item, originalIndex }));
    } else {
        const prevIdx = (activeIdx - 1 + total) % total;
        const nextIdx = (activeIdx + 1) % total;
        displayList = [
            { ...list[prevIdx], originalIndex: prevIdx },
            { ...list[activeIdx], originalIndex: activeIdx },
            { ...list[nextIdx], originalIndex: nextIdx }
        ];
    }

    strip.innerHTML = '';
    const fragment = document.createDocumentFragment();
    displayList.forEach(f => {
        const fID = (f.id || '').toUpperCase();
        const isActive = (fID === activeTarget);
        const item = document.createElement('div');
        item.className = 'follower-tab-item ' + (isActive ? 'active' : '');
        item.setAttribute('data-actor-id', f.id);
        item.innerHTML = '<span class="follower-name">' + f.name + '</span>';
        item.onclick = (e) => {
            e.stopPropagation();
            selectFollower(f.id);
        };
        fragment.appendChild(item);
    });
    strip.appendChild(fragment);
}

function selectFollower(targetId) {
    if (targetId && targetId.toUpperCase() !== 'CLOUD' && targetId.toUpperCase() !== 'WARDROBE') {
        activeInventorySource = targetId;
    }
    const poseSelect = document.getElementById('studio-pose-select');
    if (poseSelect) poseSelect.value = 'IdleForceDefaultState';
    activeStudioPose = 'IdleForceDefaultState';
    if (typeof window.SelectTargetActor === 'function') {
        window.SelectTargetActor(JSON.stringify({ id: targetId }));
    }
}

let isModCatalogSummaryLoaded = false;
let modCatalogSummaries = [];
let modCatalogCache = {};
let pendingCatalogRequests = new Set();

function getCachedModItems(modName) {
    if (!modName) return null;
    if (modCatalogCache[modName] && Array.isArray(modCatalogCache[modName])) return modCatalogCache[modName];
    const targetLower = modName.toLowerCase();
    for (const key in modCatalogCache) {
        if (key.toLowerCase() === targetLower && Array.isArray(modCatalogCache[key])) {
            return modCatalogCache[key];
        }
    }
    return null;
}

function getFilteredModCatalogItems(modName) {
    const rawItems = getCachedModItems(modName) || [];
    return rawItems.filter(item => {
        if (!item || !item.name || item.name.trim().length === 0) return false;
        if (activeCategoryFilter === 'ARMORS' && item.category !== 'ARMORS') return false;
        if (activeCategoryFilter === 'JEWELRY' && item.category !== 'JEWELRY') return false;
        if (!matchesInventorySearch(item, inventorySearchQuery)) return false;
        return true;
    });
}

window.onModCatalogSummaryReady = function(argument) {
    let data = argument;
    if (typeof argument === 'string') {
        try { data = JSON.parse(argument); } catch (e) { return; }
    }
    isModCatalogSummaryLoaded = true;
    if (Array.isArray(data)) {
        modCatalogSummaries = data;
        if (activeInventorySource === 'MODEX') {
            renderInventoryList();
        }
    }
};

window.onModCatalogItemsReady = function(argument) {
    let data = argument;
    if (typeof argument === 'string') {
        try { data = JSON.parse(argument); } catch (e) { return; }
    }
    if (!data || !data.modName || !Array.isArray(data.items)) return;
    modCatalogCache[data.modName] = data.items;
    modCatalogCache[data.modName.toLowerCase()] = data.items;
    pendingCatalogRequests.delete(data.modName);
    pendingCatalogRequests.delete(data.modName.toLowerCase());
    const grid = document.getElementById('inventory-items-grid');
    const srcUpper = (activeInventorySource || '').toUpperCase();
    if (grid && (srcUpper === 'MODEX' || srcUpper === 'CATALOG')) {
        const targetModLower = data.modName.toLowerCase();
        let sec = null;
        if (grid.children) {
            for (let i = 0; i < grid.children.length; ++i) {
                const child = grid.children[i];
                const mName = child.getAttribute ? child.getAttribute('data-mod-name') : null;
                if (mName && mName.toLowerCase() === targetModLower) {
                    sec = child;
                    break;
                }
            }
        }
        if (!sec && typeof document.querySelectorAll === 'function') {
            const sections = document.querySelectorAll('.mod-group-section');
            for (let i = 0; i < sections.length; ++i) {
                const mName = sections[i].getAttribute ? sections[i].getAttribute('data-mod-name') : null;
                if (mName && mName.toLowerCase() === targetModLower) {
                    sec = sections[i];
                    break;
                }
            }
        }
        if (sec) {
            let gridContainer = null;
            if (sec.children) {
                for (let j = 0; j < sec.children.length; ++j) {
                    const c = sec.children[j];
                    if (c.classList && c.classList.contains('mod-items-grid')) {
                        gridContainer = c;
                        break;
                    }
                }
            }
            if (!gridContainer && typeof sec.querySelector === 'function') {
                gridContainer = sec.querySelector('.mod-items-grid');
            }
            if (gridContainer) {
                const sortedGroupItems = sortInventoryItems(getFilteredModCatalogItems(data.modName));
                renderModItemsIntoGrid(gridContainer, sortedGroupItems, 'MODEX');
                if (thumbnailBatchTimeout) clearTimeout(thumbnailBatchTimeout);
                thumbnailBatchTimeout = setTimeout(() => {
                    updateVisibleThumbnailsPriority();
                }, 150);
                if (typeof populateVisibleUncollapsedSections === 'function') {
                    populateVisibleUncollapsedSections();
                }
                return;
            }
        }
    }
    if (typeof populateVisibleUncollapsedSections === 'function') {
        populateVisibleUncollapsedSections();
    }
};

function getInventorySourcesList() {
    const sources = [];
    sources.push({ id: "0x00000007", type: "PLAYER", name: "🎒 Player" });

    const followers = currentWheelData.followers || [];
    followers.forEach(f => {
        const fID = (f.id || '').toUpperCase();
        if (fID !== "0X00000007" && fID !== "0X00000014") {
            sources.push({ id: f.id, type: "FOLLOWER", name: ("👤 " + (f.name || 'Follower')) });
        }
    });

    sources.push({ id: "CLOUD", type: "CLOUD", name: "📦 Wardrobe Chest" });
    sources.push({ id: "MODEX", type: "MODEX", name: "🌐 Mod Explorer" });
    return sources;
}

function cycleInvSource(delta) {
    const sources = getInventorySourcesList();
    if (sources.length === 0) return;

    let idx = sources.findIndex(s => s.id.toUpperCase() === activeInventorySource.toUpperCase());
    if (idx === -1) idx = 0;

    idx = (idx + delta + sources.length) % sources.length;
    setInventorySource(sources[idx].id);
}

function setInventorySource(sourceId) {
    if (activeInventorySource !== sourceId) {
        expandedInventoryMods.clear();
        collapsedInventoryMods.clear();
    }
    activeInventorySource = sourceId;
    if (sourceId === 'MODEX' && !inventoryGroupByMod) {
        inventoryGroupByMod = true;
        const toggle = document.getElementById('group-by-mod-toggle');
        if (toggle) toggle.checked = true;
    }
    renderInventorySourceStrip();
    renderInventoryList();
}

function renderInventorySourceStrip() {
    const strip = document.getElementById('inv-source-strip');
    if (!strip) return;

    const sources = getInventorySourcesList();
    const total = sources.length;
    if (total === 0) return;

    let activeIdx = sources.findIndex(s => s.id.toUpperCase() === activeInventorySource.toUpperCase());
    if (activeIdx === -1) {
        activeIdx = 0;
        activeInventorySource = sources[0].id;
    }

    let displayList = sources;
    if (total > 4) {
        const prevIdx = (activeIdx - 1 + total) % total;
        const nextIdx = (activeIdx + 1) % total;
        displayList = [sources[prevIdx], sources[activeIdx], sources[nextIdx]];
    }

    strip.innerHTML = '';
    const fragment = document.createDocumentFragment();
    displayList.forEach(s => {
        const btn = document.createElement('button');
        const isActive = (s.id.toUpperCase() === activeInventorySource.toUpperCase());
        btn.className = 'inv-source-chip ' + (isActive ? 'active' : '');
        btn.innerHTML = '<span>' + s.name + '</span>';
        btn.onclick = (e) => {
            e.stopPropagation();
            setInventorySource(s.id);
        };
        fragment.appendChild(btn);
    });
    strip.appendChild(fragment);
}

function matchesInventorySearch(item, query) {
    if (!query) return true;
    const q = (query || '').toLowerCase().trim();
    if (!q) return true;

    const name = (item.name || '').toLowerCase();
    const customName = (item.customName || '').toLowerCase();
    const type = (item.type || '').toLowerCase();
    const category = (item.category || '').toLowerCase();
    const armorClass = (item.armorClass || '').toLowerCase();
    const modName = (item.modName || item.plugin || item.pluginName || '').toLowerCase();
    const cleanMod = modName.replace(/\.(esp|esm|esl)$/i, '').replace(/[_\-\.]+/g, ' ').trim();

    const fullItemText = name + ' ' + customName + ' ' + type + ' ' + category + ' ' + armorClass + ' ' + modName + ' ' + cleanMod;

    const tokens = q.replace(/[_\-\.]+/g, ' ').split(/\s+/).filter(t => t.length > 0);
    if (tokens.length === 0) return true;

    return tokens.every(token => fullItemText.includes(token));
}

function handleInventorySearch(val) {
    inventorySearchQuery = (val || '').toLowerCase().trim();
    renderInventoryList();
}

function clearInventorySearch() {
    const inp = document.getElementById('inventory-search');
    if (inp) inp.value = '';
    inventorySearchQuery = '';
    renderInventoryList();
}

function handleSortChange(val) {
    activeInventorySort = val;
    renderInventoryList();
}

function getSlotOrderWeight(typeStr) {
    const t = (typeStr || '').toLowerCase();
    if (t.includes('head') || t.includes('helm') || t.includes('circlet') || t.includes('hood')) return 0;
    if (t.includes('cuirass') || t.includes('armor') || t.includes('robe') || t.includes('torso') || t.includes('body')) return 1;
    if (t.includes('gauntlet') || t.includes('glove') || t.includes('hand') || t.includes('bracer')) return 2;
    if (t.includes('boot') || t.includes('shoe') || t.includes('foot') || t.includes('feet')) return 3;
    if (t.includes('shield')) return 4;
    if (t.includes('amulet') || t.includes('necklace')) return 5;
    if (t.includes('ring')) return 6;
    return 7;
}

function getClassOrderWeight(item) {
    if (!item) return 4;
    const typeStr = item.type || '';
    const armorClass = item.armorClass || '';
    const tagsStr = (item.tags && Array.isArray(item.tags)) ? item.tags.join(' ') : '';
    const t = (typeStr + ' ' + armorClass + ' ' + tagsStr).toLowerCase();
    if (t.includes('heavy')) return 0;
    if (t.includes('light')) return 1;
    if (t.includes('cloth') || t.includes('robe')) return 2;
    if (t.includes('jewel') || t.includes('ring') || t.includes('amulet') || t.includes('circlet')) return 3;
    return 4;
}

var inventoryViewMode = 'grid';
var inventoryGroupByMod = true;
var collapsedInventoryMods = new Set();
var expandedInventoryMods = new Set();

function updateUiForPreviewsSetting() {
    const previews = isItemPreviewsEnabled();
    const panelInv = document.getElementById('panel-inventory');
    const viewSwitch = (typeof document.querySelector === 'function') ? document.querySelector('.view-mode-switch') : null;
    const inspectPrev = document.getElementById('inspect-preview-frame');
    if (panelInv) {
        panelInv.classList.toggle('v3-classic-mode', !previews);
    }
    if (viewSwitch) {
        viewSwitch.style.display = previews ? 'inline-flex' : 'none';
    }
    if (inspectPrev) {
        inspectPrev.style.display = previews ? 'flex' : 'none';
    }
    if (!previews) {
        inventoryViewMode = 'list';
        const gridEl = document.getElementById('inventory-items-grid');
        if (gridEl) {
            gridEl.classList.remove('grid-mode');
            gridEl.classList.add('list-mode');
        }
    }
}

function setInventoryViewMode(mode) {
    if (!isItemPreviewsEnabled()) {
        mode = 'list';
    }
    inventoryViewMode = mode;
    const btnGrid = document.getElementById('btn-inv-grid-mode');
    const btnList = document.getElementById('btn-inv-list-mode');
    if (btnGrid) btnGrid.classList.toggle('active', mode === 'grid');
    if (btnList) btnList.classList.toggle('active', mode === 'list');
    renderInventoryList();
}

function toggleInventoryGroupByMod(checked) {
    inventoryGroupByMod = checked;
    renderInventoryList();
}

function renderModItemsIntoGrid(gridContainer, sortedGroupItems, srcUpper = 'MODEX', limit = 48) {
    if (!gridContainer) return;
    gridContainer.style.minHeight = '';
    gridContainer.innerHTML = '';
    if (!sortedGroupItems || sortedGroupItems.length === 0) {
        gridContainer.innerHTML = '<div style="grid-column:1/-1; font-size:10.5px; color:#64748b; text-align:center; padding:12px;">No matching apparel found in this plugin.</div>';
        return;
    }

    const currentLimit = gridContainer._renderLimit || limit;
    const toRender = sortedGroupItems.slice(0, currentLimit);
    const frag = document.createDocumentFragment();
    toRender.forEach(it => {
        frag.appendChild(createInventoryItemElement(it, srcUpper));
    });
    gridContainer.appendChild(frag);

    if (sortedGroupItems.length > currentLimit) {
        const remaining = sortedGroupItems.length - currentLimit;
        const moreBtn = document.createElement('div');
        moreBtn.className = 'mod-load-more-btn';
        moreBtn.style.cssText = 'grid-column:1/-1; text-align:center; padding:8px 12px; margin:6px 0; background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.12); border-radius:4px; font-size:11px; color:#94a3b8; cursor:pointer; font-weight:600; user-select:none;';
        moreBtn.innerText = `+ Show more (${remaining} items remaining)`;
        moreBtn.onmouseenter = () => { moreBtn.style.background = 'rgba(255,255,255,0.12)'; moreBtn.style.color = '#f8fafc'; };
        moreBtn.onmouseleave = () => { moreBtn.style.background = 'rgba(255,255,255,0.06)'; moreBtn.style.color = '#94a3b8'; };
        moreBtn.onclick = (e) => {
            if (e) e.stopPropagation();
            gridContainer._renderLimit = currentLimit + 48;
            renderModItemsIntoGrid(gridContainer, sortedGroupItems, srcUpper, limit);
        };
        gridContainer.appendChild(moreBtn);
    }

    if (!inspectedItemObject && sortedGroupItems.length > 0) {
        inspectItem(sortedGroupItems[0]);
    }
}

function populateModItemsGrid(sec, gridContainer, modName) {
    if (!sec || !gridContainer || !modName) return;
    const targetLower = modName.toLowerCase();
    const srcUpper = activeInventorySource.toUpperCase();
    if (srcUpper === 'MODEX' || srcUpper === 'CATALOG') {
        const cached = getCachedModItems(modName);
        if (cached && Array.isArray(cached)) {
            const sortedGroupItems = sortInventoryItems(getFilteredModCatalogItems(modName));
            renderModItemsIntoGrid(gridContainer, sortedGroupItems, 'MODEX');
        } else {
            gridContainer.style.minHeight = '';
            if (gridContainer.children.length === 0) {
                gridContainer.innerHTML = '<div class="mod-catalog-loading">Fetching mod items...</div>';
            }
            if (pendingCatalogRequests.size >= 4) {
                return;
            }
            if (!pendingCatalogRequests.has(targetLower)) {
                pendingCatalogRequests.add(targetLower);
                setTimeout(() => {
                    pendingCatalogRequests.delete(targetLower);
                }, 4000);
                if (typeof window.RequestModCatalogItems === 'function') {
                    window.RequestModCatalogItems(JSON.stringify({ modName: modName }));
                }
            }
        }
    } else {
        const inv = getInventoryBySourceId(activeInventorySource);
        const modItems = inv.filter(item => {
            if (!item || !item.name || item.name.trim().length === 0) return false;
            if (activeCategoryFilter === 'ARMORS' && item.category !== 'ARMORS') return false;
            if (activeCategoryFilter === 'JEWELRY' && item.category !== 'JEWELRY') return false;
            if (!matchesInventorySearch(item, inventorySearchQuery)) return false;
            const m = (item.modName && item.modName.trim().length > 0 ? item.modName : 'Skyrim.esm').toLowerCase();
            return m === targetLower;
        });
        const sortedGroupItems = sortInventoryItems(modItems);
        renderModItemsIntoGrid(gridContainer, sortedGroupItems, srcUpper);
    }
}

function populateVisibleUncollapsedSections() {
    const scrollable = document.getElementById('inventory-body-scrollable') || (typeof document.querySelector === 'function' ? document.querySelector('#panel-inventory .panel-body-scrollable') : null);
    if (!scrollable) return;
    const sRect = (typeof scrollable.getBoundingClientRect === 'function') ? scrollable.getBoundingClientRect() : null;
    const sections = (typeof document.querySelectorAll === 'function') ? document.querySelectorAll('.mod-group-section') : [];
    if (!sections || sections.length === 0) return;

    if (pendingCatalogRequests.size >= 4) return;

    const toPopulate = [];
    for (let i = 0; i < sections.length; ++i) {
        const sec = sections[i];
        if (sec.classList.contains('collapsed')) continue;
        const modName = (typeof sec.getAttribute === 'function') ? sec.getAttribute('data-mod-name') : (sec.attributes ? sec.attributes['data-mod-name'] : null);
        if (!modName) continue;

        let gridContainer = null;
        if (sec.children) {
            for (let j = 0; j < sec.children.length; ++j) {
                const c = sec.children[j];
                if (c.classList && c.classList.contains('mod-items-grid')) {
                    gridContainer = c;
                    break;
                }
            }
        }
        if (!gridContainer && typeof sec.querySelector === 'function') {
            gridContainer = sec.querySelector('.mod-items-grid');
        }
        if (!gridContainer) continue;

        const needsPopulate = (gridContainer.children.length === 0 || (typeof gridContainer.querySelector === 'function' && gridContainer.querySelector('.mod-catalog-loading')));
        if (!needsPopulate) continue;

        let shouldPopulate = false;
        if (!sRect || sRect.height === 0 || typeof sec.getBoundingClientRect !== 'function') {
            shouldPopulate = (i < 4);
        } else {
            const secRect = sec.getBoundingClientRect();
            if (secRect.top > (sRect.bottom + 150)) {
                // Section is below viewport; subsequent sections in DOM flow are also below
                break;
            }
            if (secRect.bottom >= (sRect.top - 100)) {
                shouldPopulate = true;
            }
        }

        if (shouldPopulate) {
            toPopulate.push({ sec, gridContainer, modName });
            if (toPopulate.length >= 4) break;
        }
    }

    for (let i = 0; i < toPopulate.length; ++i) {
        if (pendingCatalogRequests.size >= 4) break;
        populateModItemsGrid(toPopulate[i].sec, toPopulate[i].gridContainer, toPopulate[i].modName);
    }
}

function expandAllInventoryMods() {
    collapsedInventoryMods.clear();
    const srcUpper = activeInventorySource.toUpperCase();
    const isCatalog = (srcUpper === 'MODEX' || srcUpper === 'CATALOG');

    const grid = document.getElementById('inventory-items-grid');
    let sections = [];
    if (grid && grid.children && grid.children.length > 0) {
        sections = Array.from(grid.children).filter(c => c.classList && c.classList.contains('mod-group-section'));
    }
    if (!sections || sections.length === 0) {
        sections = (typeof document.querySelectorAll === 'function') ? Array.from(document.querySelectorAll('.mod-group-section')) : [];
    }
    const docSections = (typeof document.querySelectorAll === 'function') ? Array.from(document.querySelectorAll('.mod-group-section')) : [];

    if (isCatalog) {
        expandedInventoryMods.clear();
        if (sections && sections.length > 0) {
            for (let i = 0; i < sections.length; ++i) {
                const sec = sections[i];
                const modName = (typeof sec.getAttribute === 'function') ? sec.getAttribute('data-mod-name') : (sec.attributes ? sec.attributes['data-mod-name'] : null);
                if (!modName) continue;

                sec.classList.remove('collapsed');
                expandedInventoryMods.add(modName);

                let gridContainer = null;
                if (sec.children) {
                    for (let j = 0; j < sec.children.length; ++j) {
                        const c = sec.children[j];
                        if (c.classList && c.classList.contains('mod-items-grid')) {
                            gridContainer = c;
                            break;
                        }
                    }
                }
                if (!gridContainer && typeof sec.querySelector === 'function') {
                    gridContainer = sec.querySelector('.mod-items-grid');
                }
                if (gridContainer) {
                    const cached = getCachedModItems(modName);
                    if (cached && Array.isArray(cached)) {
                        if (i < 2 && (gridContainer.children.length === 0 || (typeof gridContainer.querySelector === 'function' && gridContainer.querySelector('.mod-catalog-loading')))) {
                            populateModItemsGrid(sec, gridContainer, modName);
                        }
                    } else if (gridContainer.children.length === 0) {
                        gridContainer.style.minHeight = '140px';
                        gridContainer.innerHTML = '<div class="mod-catalog-loading">Fetching mod items...</div>';
                    }
                }
                if (typeof observeModSection === 'function') {
                    observeModSection(sec);
                }
            }

            if (docSections.length > 0) {
                docSections.forEach(sec => sec.classList.remove('collapsed'));
            }

            if (typeof showSaveToast === 'function') {
                showSaveToast(`Expanded all ${sections.length} mods`);
            }

            if (typeof requestAnimationFrame === 'function') {
                requestAnimationFrame(() => {
                    populateVisibleUncollapsedSections();
                });
            } else {
                populateVisibleUncollapsedSections();
            }
            return;
        } else if (modCatalogSummaries) {
            for (let i = 0; i < modCatalogSummaries.length; ++i) {
                if (modCatalogSummaries[i].modName) expandedInventoryMods.add(modCatalogSummaries[i].modName);
            }
            renderInventoryList();
            return;
        }
    } else {
        // Standard inventory (Player, Follower, Wardrobe)
        expandedInventoryMods.clear();
        if (sections && sections.length > 0) {
            for (let i = 0; i < sections.length; ++i) {
                const sec = sections[i];
                const modName = (typeof sec.getAttribute === 'function') ? sec.getAttribute('data-mod-name') : (sec.attributes ? sec.attributes['data-mod-name'] : null);
                if (!modName) continue;

                sec.classList.remove('collapsed');
                expandedInventoryMods.add(modName);

                let gridContainer = null;
                if (sec.children) {
                    for (let j = 0; j < sec.children.length; ++j) {
                        const c = sec.children[j];
                        if (c.classList && c.classList.contains('mod-items-grid')) {
                            gridContainer = c;
                            break;
                        }
                    }
                }
                if (!gridContainer && typeof sec.querySelector === 'function') {
                    gridContainer = sec.querySelector('.mod-items-grid');
                }
                if (gridContainer && gridContainer.children.length === 0 && i < 4) {
                    populateModItemsGrid(sec, gridContainer, modName);
                }
            }

            populateVisibleUncollapsedSections();
            updateVisibleThumbnailsPriority();
            return;
        } else {
            const inv = getInventoryBySourceId(activeInventorySource);
            inv.forEach(it => {
                const m = it.modName && it.modName.trim().length > 0 ? it.modName : 'Skyrim.esm';
                expandedInventoryMods.add(m);
            });
            renderInventoryList();
            return;
        }
    }
}

function collapseAllInventoryMods() {
    expandedInventoryMods.clear();
    collapsedInventoryMods.clear();
    const sections = (typeof document.querySelectorAll === 'function')
        ? Array.from(document.querySelectorAll('.mod-group-section'))
        : [];

    if (sections && sections.length > 0) {
        sections.forEach(sec => {
            sec.classList.add('collapsed');
            const modName = (typeof sec.getAttribute === 'function') ? sec.getAttribute('data-mod-name') : (sec.attributes ? sec.attributes['data-mod-name'] : null);
            if (modName) collapsedInventoryMods.add(modName);
        });
        return;
    }
    renderInventoryList();
}

function toggleInventoryModCollapse(modName) {
    if (!modName) return;
    const targetLower = modName.toLowerCase();
    const sections = (typeof document.querySelectorAll === 'function') ? document.querySelectorAll('.mod-group-section') : [];
    let sec = null;
    for (let i = 0; i < sections.length; ++i) {
        const mName = (typeof sections[i].getAttribute === 'function') ? sections[i].getAttribute('data-mod-name') : (sections[i].attributes ? sections[i].attributes['data-mod-name'] : null);
        if (mName && mName.toLowerCase() === targetLower) {
            sec = sections[i];
            break;
        }
    }

    if (!sec) {
        renderInventoryList();
        return;
    }

    const isCurrentlyCollapsed = sec.classList.contains('collapsed');
    if (isCurrentlyCollapsed) {
        sec.classList.remove('collapsed');
        collapsedInventoryMods.delete(modName);
        for (const m of collapsedInventoryMods) {
            if (m.toLowerCase() === targetLower) collapsedInventoryMods.delete(m);
        }
        expandedInventoryMods.add(modName);

        let gridContainer = null;
        if (sec.children) {
            for (let j = 0; j < sec.children.length; ++j) {
                const c = sec.children[j];
                if (c.classList && c.classList.contains('mod-items-grid')) {
                    gridContainer = c;
                    break;
                }
            }
        }
        if (!gridContainer && typeof sec.querySelector === 'function') {
            gridContainer = sec.querySelector('.mod-items-grid');
        }
        if (gridContainer && (gridContainer.children.length === 0 || (typeof gridContainer.querySelector === 'function' && gridContainer.querySelector('.mod-catalog-loading')))) {
            pendingCatalogRequests.delete(modName);
            pendingCatalogRequests.delete(targetLower);
            populateModItemsGrid(sec, gridContainer, modName);
        }
        updateVisibleThumbnailsPriority();
    } else {
        sec.classList.add('collapsed');
        expandedInventoryMods.delete(modName);
        for (const m of expandedInventoryMods) {
            if (m.toLowerCase() === targetLower) expandedInventoryMods.delete(m);
        }
        collapsedInventoryMods.add(modName);
    }
}

function sortInventoryItems(itemList) {
    return [...itemList].sort((a, b) => {
        let diff = 0;
        if (activeInventorySort === 'name_asc') {
            diff = (a.name || '').localeCompare(b.name || '');
        } else if (activeInventorySort === 'name_desc') {
            diff = (b.name || '').localeCompare(a.name || '');
        } else if (activeInventorySort === 'rating_desc') {
            diff = (Number(b.armor) || 0) - (Number(a.armor) || 0);
        } else if (activeInventorySort === 'rating_asc') {
            diff = (Number(a.armor) || 0) - (Number(b.armor) || 0);
        } else if (activeInventorySort === 'weight_asc') {
            diff = (Number(a.weight) || 0) - (Number(b.weight) || 0);
        } else if (activeInventorySort === 'weight_desc') {
            diff = (Number(b.weight) || 0) - (Number(a.weight) || 0);
        } else if (activeInventorySort === 'value_desc') {
            diff = (Number(b.value) || 0) - (Number(a.value) || 0);
        } else if (activeInventorySort === 'value_asc') {
            diff = (Number(a.value) || 0) - (Number(b.value) || 0);
        } else if (activeInventorySort === 'equipped_first') {
            diff = (b.equipped ? 1 : 0) - (a.equipped ? 1 : 0);
        } else if (activeInventorySort === 'enchanted_first') {
            const aEnch = (a.tags && Array.isArray(a.tags) && a.tags.includes('enchanted')) ? 1 : 0;
            const bEnch = (b.tags && Array.isArray(b.tags) && b.tags.includes('enchanted')) ? 1 : 0;
            diff = bEnch - aEnch;
        } else if (activeInventorySort === 'slot_order') {
            diff = getSlotOrderWeight(a.type) - getSlotOrderWeight(b.type);
        } else if (activeInventorySort === 'class_order') {
            diff = getClassOrderWeight(a) - getClassOrderWeight(b);
        }

        if (diff !== 0) return diff;
        return (a.name || '').localeCompare(b.name || '');
    });
}

function isItemPreviewsEnabled() {
    if (typeof currentWheelData !== 'undefined' && currentWheelData && currentWheelData.showItemPreviews !== undefined) {
        return currentWheelData.showItemPreviews !== false;
    }
    if (typeof safeGetStorage === 'function') {
        return safeGetStorage('outfit_wheeler_show_previews', 'true') !== 'false';
    }
    return true;
}

function getItemSlotSvg(item) {
    const rawType = ((item && (item.type || item.category || '')) || '').toLowerCase();
    const rawSlot = ((item && (item.slot || item.equipSlot || '')) || '').toLowerCase();
    const rawName = ((item && item.name) || '').toLowerCase();
    const t = rawType + ' ' + rawSlot + ' ' + rawName;

    if (t.includes('shield')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>';
    }
    if (t.includes('head') || t.includes('helm') || t.includes('circlet') || t.includes('hood') || t.includes('crown') || t.includes('mask') || t.includes('hat') || t.includes('hair')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M12 3a9 9 0 0 0-9 9v3a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-2h8v2a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a9 9 0 0 0-9-9z"/><path d="M7 13h10"/><path d="M12 3v5"/></svg>';
    }
    if (t.includes('glove') || t.includes('gauntlet') || t.includes('hand') || t.includes('bracer') || t.includes('wrist') || t.includes('arm')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M7 11V5a2 2 0 0 1 4 0v6"/><path d="M11 5a2 2 0 0 1 4 0v6"/><path d="M15 7a2 2 0 0 1 4 0v5a7 7 0 0 1-7 7h-2a5 5 0 0 1-5-5v-3a2 2 0 0 1 4 0"/></svg>';
    }
    if (t.includes('boot') || t.includes('shoe') || t.includes('foot') || t.includes('feet') || t.includes('greave') || t.includes('sandal') || t.includes('heel') || t.includes('calf') || t.includes('calves')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M7 4h7v9l3 3v4H5v-2a4 4 0 0 1 2-3.5V4z"/><path d="M5 18h12"/></svg>';
    }
    if (t.includes('ring') || t.includes('band')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><circle cx="12" cy="14" r="6"/><path d="M9 8l3-4 3 4H9z"/></svg>';
    }
    if (t.includes('amulet') || t.includes('necklace') || t.includes('pendant') || t.includes('choker') || t.includes('collar') || t.includes('locket')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><circle cx="12" cy="7" r="4"/><path d="M12 11v6"/><path d="M9.5 17h5l-2.5 4-2.5-4z"/></svg>';
    }
    if (t.includes('pant') || t.includes('trousers') || t.includes('skirt') || t.includes('legging') || t.includes('bottom') || t.includes('thong') || t.includes('panties') || t.includes('underwear')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M6 4h12l1 16h-5l-2-8-2 8H5L6 4z"/></svg>';
    }
    if (t.includes('robe') || t.includes('cloak') || t.includes('cape') || t.includes('dress') || t.includes('gown')) {
        return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M9 4l3 2 3-2 3 4-2 14H8L6 8l3-4z"/><path d="M12 6v16"/></svg>';
    }
    return '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" class="item-slot-svg"><path d="M16 3H8l-2 5 2 2v11h8V10l2-2-2-5z"/><path d="M12 3v18"/><path d="M8 8h8"/></svg>';
}

function getItemTypeIcon(item) {
    const rawType = ((item && (item.type || item.category || '')) || '').toLowerCase();
    const rawSlot = ((item && (item.slot || item.equipSlot || '')) || '').toLowerCase();
    const rawName = ((item && item.name) || '').toLowerCase();
    const t = rawType + ' ' + rawSlot + ' ' + rawName;
    if (t.includes('head') || t.includes('helm') || t.includes('circlet') || t.includes('hood') || t.includes('crown') || t.includes('mask')) return '🪖';
    if (t.includes('glove') || t.includes('gauntlet') || t.includes('hand') || t.includes('bracer')) return '🧤';
    if (t.includes('boot') || t.includes('shoe') || t.includes('foot') || t.includes('feet')) return '👢';
    if (t.includes('ring')) return '💍';
    if (t.includes('amulet') || t.includes('necklace') || t.includes('pendant')) return '📿';
    if (t.includes('shield')) return '🛡️';
    if (t.includes('pant') || t.includes('trousers') || t.includes('skirt') || t.includes('legging')) return '🩲';
    if (t.includes('dress') || t.includes('robe') || t.includes('cloak')) return '👗';
    return '🥋';
}

if (typeof window !== 'undefined') {
    window.isItemPreviewsEnabled = isItemPreviewsEnabled;
    window.getItemTypeIcon = getItemTypeIcon;
    window.getItemSlotSvg = getItemSlotSvg;
}

const pendingThumbnailRequests = new Set();
const requestedThumbnailRequests = new Set();
const failedThumbnailRequests = new Set();
let thumbnailBatchTimeout = null;
let visibleThumbnailsScheduled = false;

function queueThumbnailRequest(rawID, highPriority = false) {
    if (!isItemPreviewsEnabled()) return;
    if (!rawID || typeof window.RequestThumbnail !== 'function') return;
    const idNum = Number(rawID);
    if (!idNum || idNum === 0 || failedThumbnailRequests.has(idNum)) return;
    if (!highPriority && requestedThumbnailRequests.has(idNum)) return;

    if (highPriority) {
        requestedThumbnailRequests.add(idNum);
        window.RequestThumbnail(JSON.stringify({ rawIDs: [idNum], highPriority: true }));
        return;
    }

    if (pendingThumbnailRequests.has(idNum)) return;
    pendingThumbnailRequests.add(idNum);

    if (isInventoryScrolling) return;

    if (!thumbnailBatchTimeout) {
        thumbnailBatchTimeout = setTimeout(() => {
            thumbnailBatchTimeout = null;
            if (isInventoryScrolling) return;
            flushPendingThumbnails();
        }, 120);
    }
}

function flushPendingThumbnails() {
    if (thumbnailBatchTimeout) {
        clearTimeout(thumbnailBatchTimeout);
        thumbnailBatchTimeout = null;
    }
    if (pendingThumbnailRequests.size > 0) {
        const allIds = Array.from(pendingThumbnailRequests).slice(0, 6);
        for (let i = 0; i < allIds.length; i++) {
            requestedThumbnailRequests.add(allIds[i]);
        }
        pendingThumbnailRequests.clear();
        if (typeof window.RequestThumbnail === 'function') {
            window.RequestThumbnail(JSON.stringify({ rawIDs: allIds, highPriority: false }));
        }
    } else {
        requestVisibleThumbnails();
    }
}

let thumbnailIntersectionObserver = null;
function observeThumbnailCard(card) {
    if (!isItemPreviewsEnabled()) return;
    if (typeof window === 'undefined' || typeof window.IntersectionObserver !== 'function') return;
    if (!thumbnailIntersectionObserver) {
        const scrollable = document.getElementById('inventory-body-scrollable') || (typeof document.querySelector === 'function' ? document.querySelector('#panel-inventory .panel-body-scrollable') : null);
        thumbnailIntersectionObserver = new IntersectionObserver((entries) => {
            if (isInventoryScrolling || isMouseLeftDown) return;
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const el = entry.target;
                    const lazyImg = el.querySelector('.item-thumb-img[data-src]');
                    if (lazyImg) {
                        lazyImg.onload = () => {
                            lazyImg.style.display = 'block';
                            const plc = el.querySelector('.item-thumb-placeholder');
                            if (plc) plc.style.display = 'none';
                        };
                        lazyImg.onerror = () => {
                            lazyImg.style.display = 'none';
                            const plc = el.querySelector('.item-thumb-placeholder');
                            if (plc) plc.style.display = 'flex';
                        };
                        lazyImg.src = lazyImg.getAttribute('data-src');
                        lazyImg.removeAttribute('data-src');
                        thumbnailIntersectionObserver.unobserve(el);
                    }
                    const rawId = el.getAttribute('data-raw-id');
                    if (rawId) {
                        const img = el.querySelector('.item-thumb-img');
                        if (!img || !img.src) {
                            queueThumbnailRequest(Number(rawId), false);
                            if (thumbnailIntersectionObserver) {
                                thumbnailIntersectionObserver.unobserve(el);
                            }
                        }
                    }
                }
            });
        }, { root: scrollable, rootMargin: '80px 0px' });
    }
    thumbnailIntersectionObserver.observe(card);
}

let modSectionObserver = null;
function observeModSection(sec) {
    if (typeof window === 'undefined' || typeof window.IntersectionObserver !== 'function' || !sec) return;
    if (!modSectionObserver) {
        const scrollable = document.getElementById('inventory-body-scrollable') || (typeof document.querySelector === 'function' ? document.querySelector('#panel-inventory .panel-body-scrollable') : null);
        modSectionObserver = new IntersectionObserver((entries) => {
            if (isInventoryScrolling || isMouseLeftDown) return;
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const secEl = entry.target;
                    if (!secEl.classList.contains('collapsed')) {
                        const modName = (typeof secEl.getAttribute === 'function') ? secEl.getAttribute('data-mod-name') : (secEl.attributes ? secEl.attributes['data-mod-name'] : null);
                        let gridContainer = null;
                        if (secEl.children) {
                            for (let j = 0; j < secEl.children.length; ++j) {
                                const c = secEl.children[j];
                                if (c.classList && c.classList.contains('mod-items-grid')) {
                                    gridContainer = c;
                                    break;
                                }
                            }
                        }
                        if (!gridContainer && typeof secEl.querySelector === 'function') {
                            gridContainer = secEl.querySelector('.mod-items-grid');
                        }
                        if (modName && gridContainer && (gridContainer.children.length === 0 || (typeof gridContainer.querySelector === 'function' && gridContainer.querySelector('.mod-catalog-loading')))) {
                            if (pendingCatalogRequests.size < 4) {
                                populateModItemsGrid(secEl, gridContainer, modName);
                            }
                        }
                    }
                }
            });
        }, { root: scrollable, rootMargin: '100px 0px' });
    }
    modSectionObserver.observe(sec);
}

function updateVisibleThumbnailsPriority() {
    requestVisibleThumbnails();
}

function queueAllInventoryThumbnailsBackground() {}

function autoUnequipConflictingSlots(newItem) {
    if (!newItem || !newItem.type || !currentWheelData) return;
    const exclusiveTypes = ['Helmet', 'Cuirass', 'Boots', 'Gauntlets', 'Shield'];
    if (!exclusiveTypes.includes(newItem.type)) return;
    const srcUpper = (currentWheelData.activeTargetActorId || '0x00000007').toUpperCase();
    let inv = [];
    if (srcUpper === '0X00000007' || srcUpper === '0X00000014' || srcUpper === 'PLAYER') {
        inv = currentWheelData.playerInventory || currentWheelData.inventory || [];
    } else if (currentWheelData.actorInventories && currentWheelData.actorInventories[srcUpper]) {
        inv = currentWheelData.actorInventories[srcUpper];
    } else if (currentWheelData.followerInventory) {
        inv = currentWheelData.followerInventory;
    }
    const newRaw = Number(newItem.rawID) || (newItem.id ? parseInt(newItem.id, 16) : 0) || 0;
    inv.forEach(other => {
        if (!other || !other.equipped || other.type !== newItem.type) return;
        const oRaw = Number(other.rawID) || (other.id ? parseInt(other.id, 16) : 0) || 0;
        if (newRaw && oRaw && newRaw === oRaw) return;
        other.equipped = false;
        const oSelector = oRaw ? `[data-raw-id="${oRaw}"]` : `[data-form-id="${other.id}"]`;
        document.querySelectorAll(oSelector).forEach(c => {
            c.classList.remove('is-equipped-item');
            const b = c.querySelector('.item-badges-strip .tag-equipped');
            if (b) b.remove();
        });
    });
    if (typeof modCatalogCache === 'object' && modCatalogCache !== null) {
        for (let modKey in modCatalogCache) {
            let list = modCatalogCache[modKey];
            if (Array.isArray(list)) {
                list.forEach(other => {
                    if (!other || !other.equipped || other.type !== newItem.type) return;
                    const oRaw = Number(other.rawID) || (other.id ? parseInt(other.id, 16) : 0) || 0;
                    if (newRaw && oRaw && newRaw === oRaw) return;
                    other.equipped = false;
                });
            }
        }
    }
}

window.onItemEquipToggled = function(argument) {
    let data = argument;
    if (typeof argument === 'string') {
        try { data = JSON.parse(argument); } catch (e) { return; }
    }
    if (!data) return;
    const rawID = Number(data.rawID) || 0;
    const formIDStr = (data.id || '').toUpperCase();
    const isEquipped = !!data.equipped;

    const updateInList = (list) => {
        if (!Array.isArray(list)) return;
        list.forEach(it => {
            const rId = Number(it.rawID) || (it.id ? parseInt(it.id, 16) : 0) || 0;
            const fStr = (it.id || '').toUpperCase();
            if ((rawID && rId === rawID) || (formIDStr && fStr === formIDStr)) {
                it.equipped = isEquipped;
            }
        });
    };
    if (currentWheelData) {
        updateInList(currentWheelData.playerInventory);
        updateInList(currentWheelData.inventory);
        updateInList(currentWheelData.followerInventory);
        if (currentWheelData.actorInventories) {
            for (let k in currentWheelData.actorInventories) {
                updateInList(currentWheelData.actorInventories[k]);
            }
        }
    }
    if (typeof modCatalogCache === 'object' && modCatalogCache !== null) {
        for (let modKey in modCatalogCache) {
            updateInList(modCatalogCache[modKey]);
        }
    }

    const selector = rawID ? `[data-raw-id="${rawID}"]` : `[data-form-id="${formIDStr}"]`;
    document.querySelectorAll(selector).forEach(card => {
        card.classList.toggle('is-equipped-item', isEquipped);
        let bStrip = card.querySelector('.item-badges-strip');
        if (bStrip) {
            let eqBadge = bStrip.querySelector('.tag-equipped');
            if (isEquipped && !eqBadge) {
                const span = document.createElement('span');
                span.className = 'badge-tag tag-equipped';
                span.style = 'background:rgba(16,185,129,0.2); color:#10b981; border:1px solid #10b981; font-weight:bold;';
                span.innerText = 'EQUIPPED';
                bStrip.prepend(span);
            } else if (!isEquipped && eqBadge) {
                eqBadge.remove();
            }
        }
    });
};

let cachedEquippedFormIdsSet = null;
let cachedEquippedActorId = null;

function invalidateEquippedItemCache() {
    cachedEquippedFormIdsSet = null;
    cachedEquippedActorId = null;
}

function getEquippedItemIdsSet() {
    if (!currentWheelData) return new Set();
    const srcUpper = (currentWheelData.activeTargetActorId || '0x00000007').toUpperCase();
    if (cachedEquippedFormIdsSet && cachedEquippedActorId === srcUpper) {
        return cachedEquippedFormIdsSet;
    }
    let inv = [];
    if (srcUpper === '0X00000007' || srcUpper === '0X00000014' || srcUpper === 'PLAYER') {
        inv = currentWheelData.playerInventory || currentWheelData.inventory || [];
    } else if (currentWheelData.actorInventories && currentWheelData.actorInventories[srcUpper]) {
        inv = currentWheelData.actorInventories[srcUpper];
    } else if (currentWheelData.followerInventory) {
        inv = currentWheelData.followerInventory;
    }
    const set = new Set();
    inv.forEach(x => {
        if (!x || !x.equipped) return;
        const xRaw = Number(x.rawID) || (x.id ? parseInt(x.id, 16) : 0) || 0;
        if (xRaw) set.add(xRaw);
        if (x.id) set.add(String(x.id).toUpperCase());
    });
    cachedEquippedFormIdsSet = set;
    cachedEquippedActorId = srcUpper;
    return set;
}

function isItemEquippedOnCurrentActor(rawID, formIDStr) {
    const set = getEquippedItemIdsSet();
    const rId = Number(rawID) || 0;
    if (rId && set.has(rId)) return true;
    if (formIDStr && set.has(String(formIDStr).toUpperCase())) return true;
    return false;
}

function createInventoryItemElement(item, srcUpper) {
    const rawIdVal = item.rawID || item.formID || (item.id ? parseInt(item.id, 16) : 0) || 0;
    const previewsEnabled = isItemPreviewsEnabled();
    const isGrid = previewsEnabled && (inventoryViewMode === 'grid');

    if (srcUpper === 'MODEX' || srcUpper === 'CATALOG' || item.source === 'MODEX') {
        item.equipped = isItemEquippedOnCurrentActor(rawIdVal, item.id);
    }

    const card = document.createElement('div');
    card.className = (isGrid ? 'item-card ' : 'item-row-card ') + (item.equipped ? 'is-equipped-item' : '');
    card._item = item;

    let badgeTagsHtml = '';
    if (item.equipped) badgeTagsHtml += '<span class="badge-tag tag-equipped" style="background:rgba(16,185,129,0.2); color:#10b981; border:1px solid #10b981; font-weight:bold;">EQUIPPED</span>';
    if (item.source === 'CLOUD' || srcUpper === 'CLOUD') badgeTagsHtml += '<span class="badge-tag" style="background:rgba(82,165,245,0.2); color:#52a5f5; border:1px solid #52a5f5;">WARDROBE</span>';
    if (item.tags && item.tags.includes('enchanted')) badgeTagsHtml += '<span class="badge-tag tag-enchanted">ENCH</span>';
    if (item.tags && item.tags.includes('stolen')) badgeTagsHtml += '<span class="badge-tag tag-stolen">STOLEN</span>';

    if (!previewsEnabled) {
        // EXACT VERSION 3.0 ROW FORMAT (Clean, compact, no thumbnail box or placeholder)
        let statPillText = 'ARM ' + (item.armor || 0);
        card.innerHTML = `
            <div class="item-left-info">
                <div class="item-meta-titles">
                    <span class="item-name-text">${item.name}</span>
                    <span class="item-sub-desc">${item.type || 'Apparel'} &bull; Wt ${Number(item.weight || 0).toFixed(1)} &bull; ${item.value || 0}g</span>
                </div>
            </div>
            <div class="item-right-stats">
                <div class="item-badges-strip">${badgeTagsHtml}</div>
                <span class="item-stat-pill">${statPillText}</span>
            </div>
        `;
    } else if (isGrid) {
        const hasThumb = item.thumbnail && item.thumbnail.length > 0;
        const slotSvg = getItemSlotSvg(item);
        const thumbHtml = hasThumb
            ? `<img class="item-thumb-img" data-src="${item.thumbnail}" alt="" style="display:none;"><span class="item-thumb-placeholder">${slotSvg}</span>`
            : `<span class="item-thumb-placeholder">${slotSvg}</span>`;
        card.innerHTML = `
            <span class="card-type-tag">${item.type || 'Armor'}</span>
            <span class="card-equipped-indicator"></span>
            <div class="item-badges-strip">${badgeTagsHtml}</div>
            <div class="item-thumb-wrapper">${thumbHtml}</div>
            <div class="card-name-label" title="${item.name}">${item.name}</div>
            <div class="card-stat-pill">ARM ${item.armor || 0} &bull; ${Number(item.weight || 0).toFixed(1)}w</div>
        `;
    } else {
        let statPillText = 'ARM ' + (item.armor || 0);
        const hasThumb = item.thumbnail && item.thumbnail.length > 0;
        const slotSvg = getItemSlotSvg(item);
        const thumbHtml = hasThumb
            ? `<div class="item-row-thumb-frame"><img class="item-thumb-img" data-src="${item.thumbnail}" alt="" style="display:none;"><span class="item-thumb-placeholder">${slotSvg}</span></div>`
            : `<div class="item-row-thumb-frame"><span class="item-thumb-placeholder">${slotSvg}</span></div>`;
        card.innerHTML = `
            <div class="item-left-info">
                ${thumbHtml}
                <div class="item-meta-titles">
                    <span class="item-name-text">${item.name}</span>
                    <span class="item-sub-desc">${item.type || 'Apparel'} &bull; Wt ${Number(item.weight || 0).toFixed(1)} &bull; ${item.value || 0}g</span>
                </div>
            </div>
            <div class="item-right-stats">
                <div class="item-badges-strip">${badgeTagsHtml}</div>
                <span class="item-stat-pill">${statPillText}</span>
            </div>
        `;
    }

    if (rawIdVal) card.setAttribute('data-raw-id', String(rawIdVal));
    if (item.id) card.setAttribute('data-form-id', String(item.id));
    if (item.uniqueID !== undefined) card.setAttribute('data-unique-id', String(item.uniqueID));

    if (previewsEnabled && typeof observeThumbnailCard === 'function') {
        observeThumbnailCard(card);
    }

    card.addEventListener('mousedown', (e) => {
        if (e.button === 0) {
            activeDraggedItem = item;
            isItemDragging = false;
            dragItemStartX = e.clientX;
            dragItemStartY = e.clientY;
        } else {
            activeDraggedItem = null;
            isItemDragging = false;
            if (dragGhostElement) {
                dragGhostElement.remove();
                dragGhostElement = null;
            }
        }
    });

    card.onpointerenter = () => inspectItem(item);
    card.onclick = () => {
        inspectItem(item);
        const wasEquipped = !!item.equipped;
        const newEquipped = !wasEquipped;
        item.equipped = newEquipped;
        card.classList.toggle('is-equipped-item', newEquipped);

        let bStrip = card.querySelector('.item-badges-strip');
        if (bStrip) {
            let eqBadge = bStrip.querySelector('.tag-equipped');
            if (newEquipped && !eqBadge) {
                const span = document.createElement('span');
                span.className = 'badge-tag tag-equipped';
                span.style = 'background:rgba(16,185,129,0.2); color:#10b981; border:1px solid #10b981; font-weight:bold;';
                span.innerText = 'EQUIPPED';
                bStrip.prepend(span);
            } else if (!newEquipped && eqBadge) {
                eqBadge.remove();
            }
        }
        card.classList.add('just-equipped');
        setTimeout(() => card.classList.remove('just-equipped'), 1200);

        const itemName = item.name || 'Apparel';
        if (typeof showSaveToast === 'function') {
            showSaveToast((newEquipped ? 'Equipped: ' : 'Unequipped: ') + itemName);
        }

        if (srcUpper === 'MODEX' || (item.source && item.source.toUpperCase() === 'MODEX')) {
            if (newEquipped) {
                if (typeof window.SpawnModItem === 'function') {
                    window.SpawnModItem(JSON.stringify({
                        rawID: item.rawID || item.id,
                        id: item.id,
                        targetActorId: (currentWheelData && currentWheelData.activeTargetActorId) ? currentWheelData.activeTargetActorId : '0x00000007',
                        autoEquip: true,
                        destination: 'INVENTORY'
                    }));
                }
            } else {
                if (typeof window.ToggleItemEquip === 'function') {
                    window.ToggleItemEquip(JSON.stringify({
                        rawID: item.rawID || item.id,
                        id: item.id,
                        uniqueID: item.uniqueID || 0,
                        enchantmentID: item.enchantmentID || 0,
                        customName: item.customName || '',
                        source: (currentWheelData && currentWheelData.activeTargetActorId) ? currentWheelData.activeTargetActorId : '0x00000007'
                    }));
                }
            }

            if (currentWheelData) {
                const targetActorId = (currentWheelData && currentWheelData.activeTargetActorId) ? currentWheelData.activeTargetActorId : '0x00000007';
                let targetInv = (targetActorId === '0x00000007' || targetActorId === 'PLAYER')
                    ? (currentWheelData.playerInventory || currentWheelData.inventory)
                    : (currentWheelData.followerInventory || (currentWheelData.actorInventories && currentWheelData.actorInventories[targetActorId]));
                if (!targetInv && (targetActorId === '0x00000007' || targetActorId === 'PLAYER')) {
                    targetInv = currentWheelData.playerInventory = [];
                }
                if (targetInv && Array.isArray(targetInv)) {
                    const rawIdVal = item.rawID || item.id;
                    const exists = targetInv.find(x => (x.rawID && rawIdVal && x.rawID === rawIdVal) || (x.id && item.id && x.id === item.id));
                    if (exists) {
                        exists.equipped = newEquipped;
                    } else if (newEquipped) {
                        targetInv.unshift({
                            ...item,
                            equipped: true,
                            source: targetActorId
                        });
                    }
                }
                if (newEquipped) autoUnequipConflictingSlots(item);
            }
            return;
        }

        if (newEquipped) autoUnequipConflictingSlots(item);

        if (typeof window.ToggleItemEquip === 'function') {
            window.ToggleItemEquip(JSON.stringify({
                rawID: item.rawID,
                id: item.id,
                uniqueID: item.uniqueID || 0,
                enchantmentID: item.enchantmentID || 0,
                customName: item.customName || '',
                source: item.source || activeInventorySource
            }));
        }
    };

    card.oncontextmenu = (e) => {
        e.preventDefault();
        e.stopPropagation();
        if (typeof showItemContextMenu === 'function') {
            showItemContextMenu(e.clientX, e.clientY, item);
        }
    };

    return card;
}

let inventoryRenderLimit = 48;
let scrollThumbTimeout = null;
var isInventoryScrolling = false;
if (typeof window !== 'undefined') window.isInventoryScrolling = false;
let scrollStopTimer = null;
let isMouseLeftDown = false;
let scrollChunkRafPending = false;

if (typeof window !== 'undefined' && !window._hasInventoryScrollMouseListeners) {
    window._hasInventoryScrollMouseListeners = true;
    window.addEventListener('mousedown', (e) => {
        if (e.button === 0) isMouseLeftDown = true;
    }, { capture: true, passive: true });

    window.addEventListener('mouseup', (e) => {
        if (e.button === 0) {
            isMouseLeftDown = false;
            if (isInventoryScrolling || scrollStopTimer) {
                if (scrollStopTimer) clearTimeout(scrollStopTimer);
                scrollStopTimer = setTimeout(() => {
                    isInventoryScrolling = false;
                    if (typeof window !== 'undefined') window.isInventoryScrolling = false;
                    populateVisibleUncollapsedSections();
                    flushPendingThumbnails();
                }, 250);
            }
        }
    }, { capture: true, passive: true });
}

function requestVisibleThumbnails() {
    if (typeof window === 'undefined') return;
    if (!isItemPreviewsEnabled()) return;
    if (isInventoryScrolling) return;
    const scrollable = document.getElementById('inventory-body-scrollable') || (typeof document.querySelector === 'function' ? document.querySelector('#panel-inventory .panel-body-scrollable') : null);
    if (!scrollable) return;
    const sRect = (typeof scrollable.getBoundingClientRect === 'function') ? scrollable.getBoundingClientRect() : null;
    if (!sRect || sRect.height === 0) return;

    const cards = (typeof scrollable.querySelectorAll === 'function') ? scrollable.querySelectorAll('.item-card, .item-row-card') : [];
    if (!cards || cards.length === 0) return;

    const visibleIds = [];
    for (let i = 0; i < cards.length; ++i) {
        const card = cards[i];
        if (card.offsetParent === null) continue;
        if (typeof card.getBoundingClientRect !== 'function') continue;
        const cRect = card.getBoundingClientRect();
        if (cRect.top > (sRect.bottom + 120)) {
            break;
        }
        if (cRect.bottom >= (sRect.top - 60) && cRect.top <= (sRect.bottom + 60)) {
            const lazyImg = card.querySelector('.item-thumb-img[data-src]');
            if (lazyImg) {
                lazyImg.src = lazyImg.getAttribute('data-src');
                lazyImg.removeAttribute('data-src');
                lazyImg.style.display = 'block';
                const plc = card.querySelector('.item-thumb-placeholder');
                if (plc) plc.style.display = 'none';
            }
            const img = card.querySelector('.item-thumb-img');
            if (!img || !img.src) {
                const rawId = card.getAttribute('data-raw-id');
                if (rawId) {
                    const idNum = Number(rawId);
                    if (idNum && !visibleIds.includes(idNum)) {
                        visibleIds.push(idNum);
                        if (visibleIds.length >= 6) break;
                    }
                }
            }
        }
    }

    if (visibleIds.length > 0 && typeof window.RequestThumbnail === 'function') {
        window.RequestThumbnail(JSON.stringify({ rawIDs: visibleIds, highPriority: false }));
    }
}

function setupInventoryScrollChunking() {
    const scrollable = document.getElementById('inventory-body-scrollable') || (typeof document.querySelector === 'function' ? document.querySelector('#panel-inventory .panel-body-scrollable') : null);
    if (scrollable && !scrollable._hasChunkScroll) {
        scrollable._hasChunkScroll = true;
        scrollable.addEventListener('scroll', () => {
            isInventoryScrolling = true;
            if (typeof window !== 'undefined') window.isInventoryScrolling = true;
            pendingThumbnailRequests.clear();
            if (thumbnailBatchTimeout) {
                clearTimeout(thumbnailBatchTimeout);
                thumbnailBatchTimeout = null;
            }
            if (scrollStopTimer) clearTimeout(scrollStopTimer);
            scrollStopTimer = setTimeout(() => {
                if (isMouseLeftDown) return;
                isInventoryScrolling = false;
                if (typeof window !== 'undefined') window.isInventoryScrolling = false;
                populateVisibleUncollapsedSections();
                flushPendingThumbnails();
            }, 250);

            if (!inventoryGroupByMod && !scrollChunkRafPending) {
                scrollChunkRafPending = true;
                requestAnimationFrame(() => {
                    scrollChunkRafPending = false;
                    if (!scrollable) return;
                    if (scrollable.scrollTop + scrollable.clientHeight >= scrollable.scrollHeight - 300) {
                        const inv = getInventoryBySourceId(activeInventorySource);
                        if (inventoryRenderLimit < inv.length) {
                            inventoryRenderLimit += 48;
                            renderInventoryList(true);
                        }
                    }
                });
            }
        }, { passive: true });
    }
}

function renderInventoryList(isChunkAppend) {
    if (!isChunkAppend) {
        inventoryRenderLimit = 48;
        if (thumbnailIntersectionObserver) {
            thumbnailIntersectionObserver.disconnect();
            thumbnailIntersectionObserver = null;
        }
        if (modSectionObserver) {
            modSectionObserver.disconnect();
            modSectionObserver = null;
        }
    }
    setupInventoryScrollChunking();
    updateUiForPreviewsSetting();

    const grid = document.getElementById('inventory-items-grid');
    if (!grid) return;
    grid.className = 'inventory-items-grid ' + (inventoryViewMode === 'grid' ? 'grid-mode' : 'list-mode');

    const inv = getInventoryBySourceId(activeInventorySource);
    const srcUpper = activeInventorySource.toUpperCase();

    let items = inv.filter(item => {
        if (!item.name || item.name.trim().length === 0) return false;
        if (activeCategoryFilter === 'ARMORS' && item.category !== 'ARMORS') return false;
        if (activeCategoryFilter === 'JEWELRY' && item.category !== 'JEWELRY') return false;
        if (!matchesInventorySearch(item, inventorySearchQuery)) return false;
        return true;
    });

    const cAll = document.getElementById('count-all');
    const cArm = document.getElementById('count-armors');
    const cJwl = document.getElementById('count-jewelry');

    if (cAll) cAll.innerText = inv.length;
    if (cArm) cArm.innerText = inv.filter(x => x.category === 'ARMORS').length;
    if (cJwl) cJwl.innerText = inv.filter(x => x.category === 'JEWELRY').length;

    const statWeight = document.getElementById('stat-carry-weight');
    if (statWeight) {
        if (srcUpper === 'CLOUD' || srcUpper === 'WARDROBE' || srcUpper === 'MODEX' || srcUpper === 'CATALOG') {
            statWeight.innerText = '-- / --';
        } else {
            const fList = (currentWheelData && currentWheelData.followers) ? currentWheelData.followers : [];
            const found = fList.find(f => (f.id || '').toUpperCase() === srcUpper || (srcUpper === 'PLAYER' && (f.type === 'PLAYER' || (f.id || '').toUpperCase() === '0X00000007')));
            if (found && found.weight) {
                statWeight.innerText = found.weight;
            } else {
                const playerActor = fList.find(f => f.type === 'PLAYER' || (f.id || '').toUpperCase() === '0X00000007');
                statWeight.innerText = (playerActor && playerActor.weight) ? playerActor.weight : '0 / 300';
            }
        }
    }

    if (srcUpper === 'MODEX' || srcUpper === 'CATALOG') {
        if (!isModCatalogSummaryLoaded && modCatalogSummaries.length === 0) {
            if (typeof window.RequestModCatalogSummary === 'function') {
                window.RequestModCatalogSummary("{}");
            }
            grid.innerHTML = '<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">Scanning installed mod plugins...</div>';
            return;
        }
        if (modCatalogSummaries.length === 0) {
            grid.innerHTML = '<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">No mod plugins with apparel items were found.</div>';
            return;
        }
        let totalCount = 0;
        modCatalogSummaries.forEach(s => { totalCount += (s.count || 0); });
        if (cAll) cAll.innerText = totalCount;
        if (cArm) cArm.innerText = totalCount;
        if (cJwl) cJwl.innerText = 0;
    } else if (items.length === 0) {
        grid.innerHTML = '';
        let emptyMsg = 'No apparel items found.';
        if (srcUpper === 'CLOUD') emptyMsg = 'Wardrobe Chest is empty.';
        grid.innerHTML = '<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">' + emptyMsg + '</div>';
        return;
    }

    if (items.length > 0 && !inspectedItemObject) {
        inspectItem(items[0]);
    }

    const scrollable = document.getElementById('inventory-body-scrollable') || (typeof document.querySelector === 'function' ? document.querySelector('#panel-inventory .panel-body-scrollable') : null);
    const savedScrollTop = scrollable ? scrollable.scrollTop : 0;

    if (inventoryGroupByMod) {
        grid.innerHTML = '';
        if (srcUpper === 'MODEX' || srcUpper === 'CATALOG') {
            let summariesToRender = modCatalogSummaries;
            if (inventorySearchQuery && inventorySearchQuery.trim().length > 0) {
                const q = inventorySearchQuery.trim().toLowerCase();
                summariesToRender = modCatalogSummaries.filter(s => {
                    if (s.modName && s.modName.toLowerCase().includes(q)) return true;
                    const cached = getCachedModItems(s.modName);
                    if (cached && cached.some(it => matchesInventorySearch(it, inventorySearchQuery))) return true;
                    return false;
                });
            }
            const hasAnyState = (expandedInventoryMods.size > 0 || collapsedInventoryMods.size > 0);
            const fragment = document.createDocumentFragment();
            let expandedCount = 0;
            for (let i = 0; i < summariesToRender.length; ++i) {
                const summary = summariesToRender[i];
                const modName = summary.modName || 'Skyrim.esm';
                const count = summary.count || 0;
                let isExpanded = false;
                if (inventorySearchQuery && inventorySearchQuery.trim().length > 0) {
                    isExpanded = (i < 12);
                } else if (hasAnyState) {
                    isExpanded = expandedInventoryMods.has(modName);
                } else {
                    isExpanded = false;
                    collapsedInventoryMods.add(modName);
                }
                const isCollapsed = !isExpanded;

                const section = document.createElement('div');
                section.className = 'mod-group-section' + (isCollapsed ? ' collapsed' : '');
                section.setAttribute('data-mod-name', modName);

                const header = document.createElement('div');
                header.className = 'mod-header-banner';
                header.onclick = () => toggleInventoryModCollapse(modName);

                const titleBox = document.createElement('div');
                titleBox.className = 'mod-title-box';

                const iconSpan = document.createElement('span');
                iconSpan.className = 'mod-icon';
                iconSpan.innerText = '📦';

                const nameSpan = document.createElement('span');
                nameSpan.className = 'mod-name';
                nameSpan.innerText = modName;

                const badgeSpan = document.createElement('span');
                badgeSpan.className = 'mod-badge';
                badgeSpan.innerText = count;

                titleBox.appendChild(iconSpan);
                titleBox.appendChild(nameSpan);
                titleBox.appendChild(badgeSpan);

                const chevronSpan = document.createElement('span');
                chevronSpan.className = 'mod-chevron';
                chevronSpan.innerHTML = '&#9660;';

                header.appendChild(titleBox);
                header.appendChild(chevronSpan);
                section.appendChild(header);

                const gridContainer = document.createElement('div');
                gridContainer.className = 'mod-items-grid';

                if (!isCollapsed) {
                    const cached = getCachedModItems(modName);
                    if (cached && Array.isArray(cached)) {
                        const sortedGroupItems = sortInventoryItems(getFilteredModCatalogItems(modName));
                        renderModItemsIntoGrid(gridContainer, sortedGroupItems, 'MODEX');
                    } else if (i < 3) {
                        gridContainer.style.minHeight = '140px';
                        gridContainer.innerHTML = '<div class="mod-catalog-loading">Fetching mod items...</div>';
                        const targetLower = modName.toLowerCase();
                        if (pendingCatalogRequests.size < 4 && !pendingCatalogRequests.has(targetLower)) {
                            pendingCatalogRequests.add(targetLower);
                            if (typeof window.RequestModCatalogItems === 'function') {
                                window.RequestModCatalogItems(JSON.stringify({ modName: modName }));
                            }
                        }
                    }
                    if (typeof observeModSection === 'function') {
                        observeModSection(section);
                    }
                }

                section.appendChild(gridContainer);
                fragment.appendChild(section);
            }
            grid.appendChild(fragment);
        } else {
            const groups = {};
            items.forEach(it => {
                const m = it.modName && it.modName.trim().length > 0 ? it.modName : 'Skyrim.esm';
                if (!groups[m]) groups[m] = [];
                groups[m].push(it);
            });

            const modNames = Object.keys(groups);
            modNames.sort((a, b) => a.localeCompare(b));

            const hasAnyState = (expandedInventoryMods.size > 0 || collapsedInventoryMods.size > 0);
            const fragment = document.createDocumentFragment();

            for (let i = 0; i < modNames.length; ++i) {
                const modName = modNames[i];
                const groupList = groups[modName];

                let isExpanded = true;
                if (inventorySearchQuery && inventorySearchQuery.trim().length > 0) {
                    isExpanded = true;
                } else if (hasAnyState) {
                    isExpanded = !collapsedInventoryMods.has(modName);
                } else {
                    isExpanded = true;
                    expandedInventoryMods.add(modName);
                }
                const isCollapsed = !isExpanded;

                const section = document.createElement('div');
                section.className = 'mod-group-section' + (isCollapsed ? ' collapsed' : '');
                section.setAttribute('data-mod-name', modName);

                const header = document.createElement('div');
                header.className = 'mod-header-banner';
                header.onclick = () => toggleInventoryModCollapse(modName);

                const titleBox = document.createElement('div');
                titleBox.className = 'mod-title-box';

                const iconSpan = document.createElement('span');
                iconSpan.className = 'mod-icon';
                iconSpan.innerText = '📦';

                const nameSpan = document.createElement('span');
                nameSpan.className = 'mod-name';
                nameSpan.innerText = modName;

                const badgeSpan = document.createElement('span');
                badgeSpan.className = 'mod-badge';
                badgeSpan.innerText = groupList.length;

                titleBox.appendChild(iconSpan);
                titleBox.appendChild(nameSpan);
                titleBox.appendChild(badgeSpan);

                const chevronSpan = document.createElement('span');
                chevronSpan.className = 'mod-chevron';
                chevronSpan.innerHTML = '&#9660;';

                header.appendChild(titleBox);
                header.appendChild(chevronSpan);
                section.appendChild(header);

                const gridContainer = document.createElement('div');
                gridContainer.className = 'mod-items-grid';

                if (!isCollapsed) {
                    const sortedGroupItems = sortInventoryItems(groupList);
                    renderModItemsIntoGrid(gridContainer, sortedGroupItems, srcUpper);
                    if (typeof observeModSection === 'function') {
                        observeModSection(section);
                    }
                }

                section.appendChild(gridContainer);
                fragment.appendChild(section);
            }

            grid.appendChild(fragment);
        }
    } else {
        if (!isChunkAppend) {
            grid.innerHTML = '';
        }
        if (srcUpper === 'MODEX' || srcUpper === 'CATALOG') {
            const rawCached = getInventoryBySourceId('MODEX');
            const allCached = rawCached.filter(item => {
                if (!item || !item.name || item.name.trim().length === 0) return false;
                if (activeCategoryFilter === 'ARMORS' && item.category !== 'ARMORS') return false;
                if (activeCategoryFilter === 'JEWELRY' && item.category !== 'JEWELRY') return false;
                if (!matchesInventorySearch(item, inventorySearchQuery)) return false;
                return true;
            });
            if (allCached.length === 0) {
                grid.innerHTML = '<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">Mod Explorer groups armors by plugin. Enable Mod Grouping to browse all installed mods.</div>';
            } else {
                const sortedItems = sortInventoryItems(allCached);
                let gridContainer = grid.querySelector('.mod-items-grid');
                if (!gridContainer || !isChunkAppend) {
                    grid.innerHTML = '';
                    gridContainer = document.createElement('div');
                    gridContainer.className = 'mod-items-grid';
                    grid.appendChild(gridContainer);
                }
                const startIndex = isChunkAppend ? Math.max(0, inventoryRenderLimit - 48) : 0;
                const toRender = sortedItems.slice(startIndex, inventoryRenderLimit);
                const frag = document.createDocumentFragment();
                toRender.forEach(it => {
                    frag.appendChild(createInventoryItemElement(it, 'MODEX'));
                });
                gridContainer.appendChild(frag);
            }
        } else {
            const sortedItems = sortInventoryItems(items);
            let gridContainer = grid.querySelector('.mod-items-grid');
            if (!gridContainer || !isChunkAppend) {
                grid.innerHTML = '';
                gridContainer = document.createElement('div');
                gridContainer.className = 'mod-items-grid';
                grid.appendChild(gridContainer);
            }
            const startIndex = isChunkAppend ? Math.max(0, inventoryRenderLimit - 48) : 0;
            const toRender = sortedItems.slice(startIndex, inventoryRenderLimit);
            const frag = document.createDocumentFragment();
            toRender.forEach(it => {
                frag.appendChild(createInventoryItemElement(it, srcUpper));
            });
            gridContainer.appendChild(frag);
        }
    }

    if (!isChunkAppend && scrollable && savedScrollTop > 0) {
        scrollable.scrollTop = savedScrollTop;
    }
    updateVisibleThumbnailsPriority();
}

function showItemContextMenu(x, y, item) {
    if (!isEditModeActive()) return;

    const menu = document.getElementById('item-context-menu');
    if (!menu) return;

    hideAllMenus();

    const title = document.getElementById('item-ctx-title');
    const targetsList = document.getElementById('item-transfer-targets-list');
    const allTargetsList = document.getElementById('item-transfer-all-targets-list');
    if (!title || !targetsList) return;

    inspectedItemObject = item;
    title.innerText = item.name || 'Item Options';
    targetsList.innerHTML = '';
    if (allTargetsList) allTargetsList.innerHTML = '';

    const slotActionBtn = document.getElementById('ctx-item-slot-action');
    if (slotActionBtn) {
        const slot = (currentWheelData && currentWheelData.slots) ? (currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex]) : null;
        let isInSelectedSlot = false;
        if (slot && slot.items) {
            const rawIdVal = item.rawID || item.formID || parseInt(item.id, 16) || 0;
            const uId = item.uniqueID || 0;
            const enchId = item.enchantmentID || 0;
            const custName = item.customName || '';
            isInSelectedSlot = slot.items.some(it => {
                const itRaw = (typeof it === 'object') ? (it.rawID || it.formID || parseInt(it.id, 16)) : (typeof it === 'number' ? it : parseInt(it, 16));
                const itUID = (typeof it === 'object') ? (it.uniqueID || 0) : 0;
                const itEnch = (typeof it === 'object') ? (it.enchantmentID || 0) : 0;
                const itName = (typeof it === 'object') ? (it.customName || '') : '';
                return itRaw === rawIdVal && itUID === uId && itEnch === enchId && itName === custName;
            });
        }
        if (isInSelectedSlot) {
            slotActionBtn.innerText = '- Remove from Current Slot';
            slotActionBtn.className = 'ctx-item ctx-danger';
            slotActionBtn.onclick = (e) => {
                if (e) {
                    e.preventDefault();
                    e.stopPropagation();
                }
                removeItemFromCurrentSlot(item.rawID || item.id, item.id);
                hideAllMenus();
            };
        } else {
            slotActionBtn.innerText = '+ Add to Current Slot';
            slotActionBtn.className = 'ctx-item';
            slotActionBtn.onclick = (e) => {
                ctxItemAddToCurrentSlot(e);
            };
        }
    }

    const currentItemSource = (activeInventorySource || item.source || '0x00000007').toUpperCase();
    if (currentItemSource === 'MODEX' || (item.source && item.source.toUpperCase() === 'MODEX')) {
        const btnWear = document.createElement('div');
        btnWear.className = 'ctx-item';
        btnWear.innerHTML = '<span>⚡ Equip (Add & Wear)</span>';
        btnWear.onclick = (e) => {
            e.preventDefault();
            e.stopPropagation();
            if (typeof window.SpawnModItem === 'function') {
                window.SpawnModItem(JSON.stringify({ rawID: item.rawID || item.id, id: item.id, autoEquip: true, destination: 'INVENTORY' }));
            }
            hideAllMenus();
        };
        targetsList.appendChild(btnWear);

        const btnAdd = document.createElement('div');
        btnAdd.className = 'ctx-item';
        btnAdd.innerHTML = '<span>🎒 Add to Inventory (1x)</span>';
        btnAdd.onclick = (e) => {
            e.preventDefault();
            e.stopPropagation();
            if (typeof window.SpawnModItem === 'function') {
                window.SpawnModItem(JSON.stringify({ rawID: item.rawID || item.id, id: item.id, autoEquip: false, destination: 'INVENTORY' }));
            }
            hideAllMenus();
        };
        targetsList.appendChild(btnAdd);

        const btnStash = document.createElement('div');
        btnStash.className = 'ctx-item';
        btnStash.innerHTML = '<span>📦 Send to Wardrobe Chest</span>';
        btnStash.onclick = (e) => {
            e.preventDefault();
            e.stopPropagation();
            if (typeof window.SpawnModItem === 'function') {
                window.SpawnModItem(JSON.stringify({ rawID: item.rawID || item.id, id: item.id, autoEquip: false, destination: 'CLOUD' }));
            }
            hideAllMenus();
        };
        targetsList.appendChild(btnStash);

        if (currentWheelData && currentWheelData.followers && currentWheelData.followers.length > 0) {
            currentWheelData.followers.forEach(fol => {
                if (fol.type !== 'PLAYER' && fol.id && fol.id.toUpperCase() !== '0X00000007' && fol.id.toUpperCase() !== '0X00000014') {
                    const btnFol = document.createElement('div');
                    btnFol.className = 'ctx-item';
                    btnFol.innerHTML = '<span>👤 Give & Equip to ' + (fol.name || 'Follower') + '</span>';
                    btnFol.onclick = (e) => {
                        e.preventDefault();
                        e.stopPropagation();
                        if (typeof window.SpawnModItem === 'function') {
                            window.SpawnModItem(JSON.stringify({ rawID: item.rawID || item.id, id: item.id, targetActorId: fol.id, autoEquip: true, destination: 'INVENTORY' }));
                        }
                        hideAllMenus();
                    };
                    targetsList.appendChild(btnFol);
                }
            });
        }
        positionContextMenu(menu, x, y);
        return;
    }

    const allSources = getInventorySourcesList();

    allSources.forEach(src => {
        const srcUpper = src.id.toUpperCase();
        if (srcUpper !== currentItemSource && srcUpper !== 'MODEX') {
            const btnSingle = document.createElement('div');
            btnSingle.className = 'ctx-item';
            btnSingle.innerHTML = '<span>' + src.name + '</span>';
            btnSingle.onmousedown = (e) => {
                e.preventDefault();
                e.stopPropagation();
                ctxItemTransfer(src.id);
            };
            btnSingle.onclick = (e) => {
                e.preventDefault();
                e.stopPropagation();
                ctxItemTransfer(src.id);
            };
            targetsList.appendChild(btnSingle);

            if (allTargetsList) {
                const btnAll = document.createElement('div');
                btnAll.className = 'ctx-item';
                btnAll.innerHTML = '<span>' + src.name + '</span>';
                btnAll.onmousedown = (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    ctxTransferAllTo(src.id);
                };
                btnAll.onclick = (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    ctxTransferAllTo(src.id);
                };
                allTargetsList.appendChild(btnAll);
            }
        }
    });

    positionContextMenu(menu, x, y);
}

function openTransferAllMenu(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }

    const menu = document.getElementById('transfer-all-context-menu');
    if (!menu) return;

    if (menu.style.display === 'flex') {
        hideAllMenus();
        return;
    }

    hideAllMenus();

    const targetsList = document.getElementById('transfer-all-targets-list');
    if (!targetsList) return;

    targetsList.innerHTML = '';
    const currentSource = (activeInventorySource || '0x00000007').toUpperCase();
    const allSources = getInventorySourcesList();

    allSources.forEach(src => {
        const sUpper = src.id.toUpperCase();
        if (sUpper !== currentSource && sUpper !== 'MODEX') {
            const btn = document.createElement('div');
            btn.className = 'ctx-item';
            btn.innerHTML = '<span>' + src.name + '</span>';
            btn.onmousedown = (ev) => {
                ev.preventDefault();
                ev.stopPropagation();
                ctxTransferAllTo(src.id);
            };
            btn.onclick = (ev) => {
                ev.preventDefault();
                ev.stopPropagation();
                ctxTransferAllTo(src.id);
            };
            targetsList.appendChild(btn);
        }
    });

    let posX = window.innerWidth / 2;
    let posY = window.innerHeight / 2;

    if (e && e.target) {
        const rect = e.target.getBoundingClientRect();
        posX = rect.left;
        posY = rect.top;
    }

    positionContextMenu(menu, posX, posY);
}

function performLocalItemTransfer(rawIdVal, fromSrc, toTargetId, uniqueID, enchantmentID, customName) {
    const srcList = getInventoryBySourceId(fromSrc);
    const dstList = getInventoryBySourceId(toTargetId);

    if (Array.isArray(srcList) && Array.isArray(dstList) && srcList !== dstList) {
        const idx = srcList.findIndex(it => {
            const itRaw = it.rawID || parseInt(it.id, 16);
            if (itRaw !== rawIdVal) return false;
            if (uniqueID !== undefined && (it.uniqueID || 0) !== uniqueID) return false;
            if (enchantmentID !== undefined && (it.enchantmentID || 0) !== enchantmentID) return false;
            if (customName !== undefined && customName !== '' && (it.customName || '') !== customName) return false;
            return true;
        });
        const targetIdx = (idx !== -1) ? idx : srcList.findIndex(it => (it.rawID || parseInt(it.id, 16)) === rawIdVal);
        if (targetIdx !== -1) {
            const [item] = srcList.splice(targetIdx, 1);
            item.source = toTargetId;
            item.equipped = false;
            dstList.unshift(item);
        }
    }
    renderInventoryList();
}

function performLocalAllTransfer(fromSrc, toTargetId) {
    const srcList = getInventoryBySourceId(fromSrc);
    const dstList = getInventoryBySourceId(toTargetId);

    if (Array.isArray(srcList) && Array.isArray(dstList) && srcList !== dstList) {
        while (srcList.length > 0) {
            const item = srcList.pop();
            item.source = toTargetId;
            item.equipped = false;
            dstList.unshift(item);
        }
    }
    renderInventoryList();
}

function ctxTransferAllTo(toTargetId) {
    const fromSrc = activeInventorySource || '0x00000007';
    performLocalAllTransfer(fromSrc, toTargetId);

    if (typeof window.TransferAllInventory === 'function') {
        window.TransferAllInventory(JSON.stringify({
            fromSource: fromSrc,
            toTarget: toTargetId
        }));
    }
    showSaveToast("Transferring all items...");
    hideAllMenus();
}

function ctxItemEquipToggle(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }
    if (inspectedItemObject && typeof window.ToggleItemEquip === 'function') {
        const rawIdVal = inspectedItemObject.rawID || inspectedItemObject.formID || parseInt(inspectedItemObject.id, 16) || 0;
        const idStrVal = inspectedItemObject.id || ('0x' + Number(rawIdVal).toString(16).toUpperCase());
        window.ToggleItemEquip(JSON.stringify({
            rawID: rawIdVal,
            id: idStrVal,
            uniqueID: inspectedItemObject.uniqueID || 0,
            enchantmentID: inspectedItemObject.enchantmentID || 0,
            customName: inspectedItemObject.customName || '',
            source: inspectedItemObject.source || activeInventorySource
        }));
    }
    hideAllMenus();
}

function ctxItemAddToCurrentSlot(e) {
    if (e) {
        e.preventDefault();
        e.stopPropagation();
    }
    addInspectedItemToCurrentSlot();
    hideAllMenus();
}

function ctxItemTransfer(toTargetId) {
    if (!inspectedItemObject) return;

    const rawIdVal = inspectedItemObject.rawID || inspectedItemObject.formID || parseInt(inspectedItemObject.id, 16) || 0;
    const idStrVal = inspectedItemObject.id || ('0x' + Number(rawIdVal).toString(16).toUpperCase());
    const fromSrc = activeInventorySource || inspectedItemObject.source || '0x00000007';
    const uniqueIDVal = inspectedItemObject.uniqueID || 0;
    const enchantmentIDVal = inspectedItemObject.enchantmentID || 0;
    const customNameVal = inspectedItemObject.customName || '';

    performLocalItemTransfer(rawIdVal, fromSrc, toTargetId, uniqueIDVal, enchantmentIDVal, customNameVal);

    if (typeof window.TransferItem === 'function') {
        window.TransferItem(JSON.stringify({
            rawID: rawIdVal,
            id: idStrVal,
            uniqueID: uniqueIDVal,
            enchantmentID: enchantmentIDVal,
            customName: customNameVal,
            fromSource: fromSrc,
            toTarget: toTargetId
        }));
    }

    showSaveToast("Transferred " + inspectedItemObject.name);
    hideAllMenus();
}

let inspectThumbnailDebounce = null;
function inspectItem(item) {
    if (!item) return;
    if (inspectedItemObject && (inspectedItemObject === item || (inspectedItemObject.id && item.id && inspectedItemObject.id === item.id && inspectedItemObject.uniqueID === item.uniqueID))) {
        return;
    }
    inspectedItemObject = item;
    const tag = document.getElementById('inspect-type-tag');
    const title = document.getElementById('inspect-title-text');
    const priLbl = document.getElementById('stat-primary-lbl');
    const priVal = document.getElementById('stat-primary-val');
    const wtVal = document.getElementById('stat-weight-val');
    const valVal = document.getElementById('stat-value-val');
    const enchBox = document.getElementById('inspect-enchantment-box');
    const enchDesc = document.getElementById('inspect-ench-desc');
    const modSrc = document.getElementById('inspect-mod-source');
    const prevImg = document.getElementById('inspect-preview-img');
    const prevPlc = document.getElementById('inspect-preview-placeholder');

    if (!tag || !title || !priLbl || !priVal) return;

    tag.innerText = item.category || 'ARMOR';
    title.innerText = item.name;

    if (modSrc) {
        modSrc.innerText = item.modName || 'Skyrim.esm';
    }

    const prevFrame = document.getElementById('inspect-preview-frame');
    if (!isItemPreviewsEnabled()) {
        if (inspectThumbnailDebounce) {
            clearTimeout(inspectThumbnailDebounce);
            inspectThumbnailDebounce = null;
        }
        if (prevFrame) prevFrame.style.display = 'none';
        if (prevImg) prevImg.style.display = 'none';
        if (prevPlc) prevPlc.style.display = 'none';
    } else {
        if (prevFrame) prevFrame.style.display = 'flex';
        if (prevImg && prevPlc) {
            if (item.thumbnail && item.thumbnail.length > 0) {
                if (inspectThumbnailDebounce) {
                    clearTimeout(inspectThumbnailDebounce);
                    inspectThumbnailDebounce = null;
                }
                prevImg.src = item.thumbnail;
                prevImg.style.display = 'block';
                prevPlc.style.display = 'none';
            } else {
                prevImg.style.display = 'none';
                prevPlc.style.display = 'block';
                prevPlc.innerHTML = getItemSlotSvg(item);
                const rawIdVal = item.rawID || item.formID || (item.id ? parseInt(item.id, 16) : 0) || 0;
                if (rawIdVal) {
                    if (inspectThumbnailDebounce) clearTimeout(inspectThumbnailDebounce);
                    inspectThumbnailDebounce = setTimeout(() => {
                        queueThumbnailRequest(rawIdVal, true);
                    }, 80);
                }
            }
        }
    }

    priLbl.innerText = 'Armor';
    priVal.innerText = item.armor || 0;
    priVal.className = 'val';

    if (wtVal) wtVal.innerText = Number(item.weight || 0).toFixed(1);
    if (valVal) valVal.innerText = Number(item.value || 0).toLocaleString() + 'g';

    if (enchBox && enchDesc) {
        if (item.enchantment) {
            enchBox.style.display = 'block';
            enchDesc.innerText = item.enchantment;
        } else {
            enchBox.style.display = 'none';
        }
    }
}

function applySingleThumbnailReady(data) {
    if (!data || !data.rawID || !data.thumbnail) return;

    const rawIdStr = String(data.rawID);
    const formIdStr = ('0x' + Number(data.rawID).toString(16).toUpperCase());

    const updateItemInList = (list) => {
        if (!Array.isArray(list)) return false;
        for (let i = 0; i < list.length; ++i) {
            const x = list[i];
            if (String(x.rawID) === rawIdStr || String(x.formID) === rawIdStr || (x.id && x.id.toUpperCase() === formIdStr)) {
                x.thumbnail = data.thumbnail;
                return true;
            }
        }
        return false;
    };

    if (currentWheelData) {
        const activeList = getInventoryBySourceId(activeInventorySource);
        if (!updateItemInList(activeList)) {
            if (!updateItemInList(currentWheelData.playerInventory)) {
                if (!updateItemInList(currentWheelData.cloudInventory)) {
                    updateItemInList(currentWheelData.followerInventory);
                }
            }
        }
    }

    if (typeof modCatalogCache === 'object' && modCatalogCache !== null) {
        for (const modKey in modCatalogCache) {
            if (updateItemInList(modCatalogCache[modKey])) break;
        }
    }

    const cards = (typeof document.querySelectorAll === 'function') ? document.querySelectorAll(`[data-raw-id="${rawIdStr}"]`) : [];
    cards.forEach(card => {
        const thumbImg = card.querySelector('.item-thumb-img');
        const thumbPlc = card.querySelector('.item-thumb-placeholder');
        if (thumbImg) {
            thumbImg.onload = () => {
                thumbImg.style.display = 'block';
                if (thumbPlc) thumbPlc.style.display = 'none';
            };
            thumbImg.onerror = () => {
                failedThumbnailRequests.add(Number(rawIdStr));
                thumbImg.style.display = 'none';
                if (thumbPlc) thumbPlc.style.display = 'flex';
            };
            thumbImg.src = data.thumbnail;
        } else if (thumbPlc) {
            const wrapper = thumbPlc.parentElement;
            if (wrapper) {
                const img = document.createElement('img');
                img.className = 'item-thumb-img';
                img.alt = '';
                img.onload = () => {
                    img.style.display = 'block';
                    thumbPlc.style.display = 'none';
                };
                img.onerror = () => {
                    failedThumbnailRequests.add(Number(rawIdStr));
                    img.remove();
                    thumbPlc.style.display = 'flex';
                };
                img.src = data.thumbnail;
                wrapper.appendChild(img);
            }
        }
    });

    const wheelSlotThumbs = (typeof document.querySelectorAll === 'function') ? document.querySelectorAll(`.wheel-slot-thumb-img[data-raw-id="${rawIdStr}"]`) : [];
    wheelSlotThumbs.forEach(img => {
        img.onerror = () => {
            failedThumbnailRequests.add(Number(rawIdStr));
            img.style.display = 'none';
            const fallback = img.parentElement ? img.parentElement.querySelector('.fallback-icon') : null;
            if (fallback) fallback.style.display = 'block';
        };
        img.src = data.thumbnail;
        img.style.display = 'block';
        const fallback = img.parentElement ? img.parentElement.querySelector('.fallback-icon') : null;
        if (fallback) fallback.style.display = 'none';
    });

    if (currentWheelData && currentWheelData.slots) {
        currentWheelData.slots.forEach(slot => {
            if (slot.items && Array.isArray(slot.items)) {
                slot.items.forEach(it => {
                    if (typeof it === 'object' && it !== null && (String(it.rawID) === rawIdStr || String(it.formID) === rawIdStr || (it.id && it.id.toUpperCase() === formIdStr))) {
                        it.thumbnail = data.thumbnail;
                    }
                });
            }
        });
    }

    if (inspectedItemObject && (String(inspectedItemObject.rawID) === rawIdStr || String(inspectedItemObject.formID) === rawIdStr || (inspectedItemObject.id && inspectedItemObject.id.toUpperCase() === formIdStr))) {
        inspectedItemObject.thumbnail = data.thumbnail;
        const inspectImg = document.getElementById('inspect-preview-img');
        const inspectPlc = document.getElementById('inspect-preview-placeholder');
        if (inspectImg && inspectPlc) {
            inspectImg.src = data.thumbnail;
            inspectImg.style.display = 'block';
            inspectPlc.style.display = 'none';
        }
    }
}

window.onThumbnailReady = function(argument) {
    let data = argument;
    if (typeof argument === 'string') {
        try { data = JSON.parse(argument); } catch (e) { return; }
    }
    if (!data) return;

    if (Array.isArray(data)) {
        data.forEach(applySingleThumbnailReady);
        return;
    }

    applySingleThumbnailReady(data);
};

window.onThumbnailFailed = function(argument) {
    const rawId = Number(argument);
    if (rawId) {
        failedThumbnailRequests.add(rawId);
        pendingThumbnailRequests.delete(rawId);
        requestedThumbnailRequests.delete(rawId);
    }
};

function clearLoadedThumbnails() {
    const clearInList = (list) => {
        if (!Array.isArray(list)) return;
        list.forEach(item => {
            if (item && item.thumbnail) delete item.thumbnail;
        });
    };
    if (window.currentWheelData) {
        clearInList(currentWheelData.playerInventory);
        clearInList(currentWheelData.cloudInventory);
        clearInList(currentWheelData.followerInventory);
        if (Array.isArray(currentWheelData.slots)) {
            currentWheelData.slots.forEach(slot => {
                if (slot && Array.isArray(slot.items)) {
                    clearInList(slot.items);
                }
            });
        }
    }
    pendingThumbnailRequests.clear();
    requestedThumbnailRequests.clear();
    failedThumbnailRequests.clear();
    if (typeof renderCurrentView === 'function') renderCurrentView();
    if (typeof updateWheelUI === 'function') updateWheelUI();
}
window.clearLoadedThumbnails = clearLoadedThumbnails;

function renderSlotsRail() {
    const rail = document.getElementById('slots-rail-list');
    if (!rail || !currentWheelData) return;
    rail.innerHTML = '';

    const slots = currentWheelData.slots || [];
    if (slots.length === 0) {
        rail.innerHTML = '<div style="font-size:10px; color:#64748b; text-align:center; padding:16px;">No slots. Click + to add.</div>';
        return;
    }

    const fragment = document.createDocumentFragment();

    slots.forEach((s, idx) => {
        const realIdx = s.index !== undefined ? s.index : idx;
        const card = document.createElement('div');
        const isSelected = (realIdx === selectedSlotIndex);
        const isWornOrEquipped = (s.isEquipped === true || s.isWorn === true);
        card.className = 'vertical-slot-card ' + (isSelected ? 'active' : '') + ' ' + (isWornOrEquipped ? 'is-equipped' : '') + ' ' + (s.isAddon ? 'is-addon' : '');
        card.setAttribute('data-slot-index', realIdx);

        let pipHtml = '';
        if (s.isAddon && isWornOrEquipped) {
            pipHtml = '<span class="slot-status-pip pip-addon-equipped">ADDON &bull; ON</span>';
        } else if (s.isAddon) {
            pipHtml = '<span class="slot-status-pip pip-addon">ADDON</span>';
        } else if (isWornOrEquipped) {
            pipHtml = '<span class="slot-status-pip pip-equipped">EQUIPPED</span>';
        }

        const displayName = (s.name && s.name.trim().length > 0) ? s.name : ('Outfit ' + (realIdx + 1));
        const itemCount = (s.items && s.items.length > 0) ? s.items.length : (s.count || 0);

        card.innerHTML = '<div class="slot-card-texts"><span class="slot-card-name">' + displayName + '</span><span class="slot-card-sub">Slot ' + (realIdx + 1) + ' &bull; ' + itemCount + ' pcs</span></div>' + pipHtml;

        card.addEventListener('mousedown', (e) => {
            if (e.button === 0) {
                activeDraggedSlotIndex = realIdx;
                isSlotDragging = false;
                slotDragStartX = e.clientX;
                slotDragStartY = e.clientY;
            }
        });

        card.onclick = (e) => {
            if (isSlotDragging) return;
            selectedSlotIndex = realIdx;
            if (activeWheelContextFilter && activeWheelContextFilter !== 'ALL') {
                const tag = (s.contextTag || '').toLowerCase().trim();
                const f = activeWheelContextFilter.toLowerCase().trim();
                let matches = tag.includes(f);
                if (f === 'city' && (tag.includes('town') || tag.includes('village'))) matches = true;
                if (f === 'home' && (tag.includes('house') || tag.includes('playerhome') || tag.includes('inn') || tag.includes('tavern') || tag.includes('bed') || tag.includes('sleep') || tag.includes('rest'))) matches = true;
                if (f === 'combat' && (tag.includes('fight') || tag.includes('war') || tag.includes('military') || tag.includes('dungeon'))) matches = true;
                if (f === 'swim' && (tag.includes('water') || tag.includes('inwater') || tag.includes('in_water'))) matches = true;
                if (!matches) {
                    activeWheelContextFilter = 'ALL';
                    const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
                    tabs.forEach(tab => {
                        if (tab.getAttribute('data-tag') === 'ALL') tab.classList.add('active');
                        else tab.classList.remove('active');
                    });
                    if (typeof updateWheelUI === 'function') updateWheelUI();
                }
            }
            renderSlotsRail();
            renderDrawerDetails();
            if (s.hasOutfit && s.items && s.items.length > 0) {
                equipOutfit(realIdx, true);
            }
        };

        card.ondblclick = (e) => {
            e.stopPropagation();
            if (s.hasOutfit && s.items && s.items.length > 0) {
                equipOutfit(realIdx, true);
            }
        };

        fragment.appendChild(card);
    });
    rail.appendChild(fragment);
}

function renderDrawerDetails() {
    const badge = document.getElementById('drawer-slot-badge');
    const nameEl = document.getElementById('drawer-outfit-name');
    const statusContainer = document.getElementById('drawer-slot-status');
    const btnFull = document.getElementById('btn-type-full');
    const btnAddon = document.getElementById('btn-type-addon');
    const btnSaveMain = document.getElementById('btn-save-drawer-gear');

    if (!badge || !nameEl || !currentWheelData) return;

    const slots = currentWheelData.slots || [];
    if (slots.length === 0) {
        badge.innerText = 'NO SLOTS';
        nameEl.innerText = 'No Slot Selected';
        if (statusContainer) statusContainer.style.display = 'none';
        renderDrawerItemsList(null);
        return;
    }

    const slot = slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || slots[selectedSlotIndex] || slots[0];
    if (!slot) return;

    badge.innerText = 'SLOT ' + ((slot.index !== undefined ? slot.index : selectedSlotIndex) + 1);
    nameEl.innerText = (slot.name && slot.name.trim().length > 0) ? slot.name : ('Outfit ' + ((slot.index !== undefined ? slot.index : selectedSlotIndex) + 1));

    if (statusContainer) {
        const isItemEquipped = (slot.isWorn === true || slot.isEquipped === true);
        if (slot.isAddon) {
            statusContainer.style.display = 'inline-block';
            if (isItemEquipped) {
                statusContainer.className = 'status-chip chip-addon-active';
                statusContainer.innerText = 'ADDON (EQUIPPED)';
            } else {
                statusContainer.className = 'status-chip chip-addon';
                statusContainer.innerText = 'ADDON (OFF)';
            }
        } else if (isItemEquipped) {
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

    if (btnSaveMain) {
        btnSaveMain.innerText = slot.isAddon ? 'Save All Worn to Addon' : 'Save Current Gear';
    }

    const btnEquipSlot = document.getElementById('btn-equip-drawer-slot');
    if (btnEquipSlot) {
        btnEquipSlot.style.display = (slot && slot.hasOutfit && slot.items && slot.items.length > 0) ? 'inline-flex' : 'none';
    }

    const currentStars = (slot && slot.preferenceStars !== undefined) ? slot.preferenceStars : 3;
    const stars = document.querySelectorAll('#stars-widget .star-icon');
    stars.forEach((s, idx) => {
        if (idx < currentStars) s.classList.add('active');
        else s.classList.remove('active');
    });

    renderDrawerContextChips();
    renderDrawerItemsList(slot);
}

function switchInspectorContextTab(tabKey, e) {
    if (e) e.stopPropagation();
    activeInspectorContextTab = tabKey;
    const btnAct = document.getElementById('tab-ctx-actions');
    const btnLoc = document.getElementById('tab-ctx-locations');
    const btnWea = document.getElementById('tab-ctx-weather');

    if (btnAct) btnAct.classList.toggle('active', tabKey === 'action');
    if (btnLoc) btnLoc.classList.toggle('active', tabKey === 'location');
    if (btnWea) btnWea.classList.toggle('active', tabKey === 'weather');

    renderDrawerContextChips();
}

function renderDrawerContextChips() {
    const box = document.getElementById('inspector-conditions-chips');
    if (!box || typeof ALL_CONDITIONS_LIST === 'undefined' || !currentWheelData || !currentWheelData.slots) return;
    box.innerHTML = '';

    const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
    if (!slot) return;
    const currentTag = (slot && slot.contextTag) ? slot.contextTag.toLowerCase() : '';

    const filtered = ALL_CONDITIONS_LIST.filter(cond => {
        if (activeInspectorContextTab === 'action') return cond.cat === 'action';
        if (activeInspectorContextTab === 'location') return cond.cat === 'generic' || cond.cat === 'unique';
        if (activeInspectorContextTab === 'weather') return cond.cat === 'modifier';
        return true;
    });

    const fragment = document.createDocumentFragment();
    filtered.forEach(cond => {
        const chip = document.createElement('span');
        const isActive = currentTag.includes(cond.id.toLowerCase());
        chip.className = 'ctx-toggle-chip ' + (isActive ? 'active' : '');
        chip.innerText = cond.label;
        chip.onclick = () => toggleContextTagDirect(cond.id);
        fragment.appendChild(chip);
    });
    box.appendChild(fragment);
}

function renderDrawerItemsList(slot) {
    const list = document.getElementById('drawer-items-list');
    if (!list) return;
    list.innerHTML = '';

    if (!slot) {
        list.innerHTML = '<div class="drawer-empty-drop-hint">No slot selected.</div>';
        return;
    }

    const items = slot.items || slot.itemIDs || [];
    if (items.length === 0) {
        list.innerHTML = '<div class="drawer-empty-drop-hint">Drag & drop items here or click \'+ Add Item\' to assign pieces.</div>';
        return;
    }

    const fragment = document.createDocumentFragment();

    items.forEach((itemEntry, itemIdx) => {
        let itemObj = null;
        if (typeof itemEntry === 'object' && itemEntry !== null) {
            const rId = itemEntry.rawID || itemEntry.formID || parseInt(itemEntry.id, 16) || 0;
            let resolvedName = itemEntry.name;
            if (!resolvedName || resolvedName === 'Apparel' || resolvedName === 'Apparel Item') {
                if (itemEntry.customName && itemEntry.customName !== 'Apparel') {
                    resolvedName = itemEntry.customName;
                } else {
                    const foundInInv = (currentWheelData.inventory || []).find(x => x.rawID === rId || x.id === itemEntry.id);
                    if (foundInInv && foundInInv.name && foundInInv.name !== 'Apparel') {
                        resolvedName = foundInInv.name;
                    }
                }
            }

            itemObj = {
                name: resolvedName || 'Apparel Item',
                rawID: rId,
                id: itemEntry.id || ('0x' + Number(rId).toString(16).toUpperCase()),
                type: itemEntry.type || itemEntry.category || 'Armor'
            };
        } else {
            const formIdNum = typeof itemEntry === 'number' ? itemEntry : (parseInt(itemEntry, 16) || 0);
            const formIdStr = typeof itemEntry === 'number' ? ('0x' + itemEntry.toString(16).toUpperCase()) : String(itemEntry);
            const found = (currentWheelData.inventory || []).find(x => x.id === formIdStr || x.rawID === formIdNum);
            itemObj = found || {
                name: 'Apparel Item (' + formIdStr + ')',
                rawID: formIdNum,
                id: formIdStr,
                type: 'Armor'
            };
        }

        const row = document.createElement('div');
        row.className = 'drawer-item-row';

        const infoDiv = document.createElement('div');
        infoDiv.className = 'drawer-item-info';
        infoDiv.innerHTML = `<span>${itemObj.name}</span>`;

        const actionsDiv = document.createElement('div');
        actionsDiv.className = 'drawer-item-actions';

        const typeTag = document.createElement('span');
        typeTag.className = 'drawer-item-slot-tag';
        typeTag.innerText = itemObj.type || 'Apparel';

        const removeBtn = document.createElement('button');
        removeBtn.className = 'btn-remove-item-cross';
        removeBtn.title = 'Remove item';
        removeBtn.innerHTML = '&#x2715;';
        removeBtn.onmousedown = (e) => {
            e.preventDefault();
            e.stopPropagation();
            removeItemFromCurrentSlot(itemObj.rawID, itemObj.id, itemIdx);
        };
        removeBtn.onclick = (e) => {
            e.preventDefault();
            e.stopPropagation();
            removeItemFromCurrentSlot(itemObj.rawID, itemObj.id, itemIdx);
        };

        actionsDiv.appendChild(typeTag);
        actionsDiv.appendChild(removeBtn);

        row.appendChild(infoDiv);
        row.appendChild(actionsDiv);
        fragment.appendChild(row);
    });
    list.appendChild(fragment);
}

function removeItemFromCurrentSlot(rawID, idStr, itemIdx) {
    const rawIdVal = rawID || parseInt(idStr, 16) || 0;
    const idStrVal = idStr || ('0x' + Number(rawIdVal).toString(16).toUpperCase());

    if (typeof window.RemoveItemFromSlot === 'function' && currentWheelData) {
        window.RemoveItemFromSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: selectedSlotIndex,
            rawID: rawIdVal,
            id: idStrVal
        }));
    }

    if (currentWheelData && currentWheelData.slots) {
        const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
        if (slot) {
            let list = slot.items || slot.itemIDs;
            if (Array.isArray(list)) {
                if (itemIdx !== undefined && itemIdx >= 0 && itemIdx < list.length) {
                    list.splice(itemIdx, 1);
                } else {
                    slot.items = list.filter(it => {
                        const itRaw = typeof it === 'object' ? (it.rawID || it.formID || parseInt(it.id, 16)) : (typeof it === 'number' ? it : parseInt(it, 16));
                        return itRaw !== rawIdVal;
                    });
                }
            }
            slot.count = (slot.items || []).length;
            renderDrawerDetails();
            renderSlotsRail();
            if (typeof updateWheelUI === 'function') updateWheelUI();
            if (typeof showSaveToast === 'function') showSaveToast('Item removed');
        }
    }
}

function addInspectedItemToCurrentSlot() {
    if (!inspectedItemObject) {
        if (typeof showSaveToast === 'function') showSaveToast("Select an item in inventory first!");
        return;
    }

    if (!currentWheelData || !currentWheelData.slots || currentWheelData.slots.length === 0) {
        if (typeof showSaveToast === 'function') showSaveToast("Create a slot first (+ Add Slot)!");
        return;
    }

    const rawIdVal = inspectedItemObject.rawID || inspectedItemObject.formID || parseInt(inspectedItemObject.id, 16) || 0;
    const idStrVal = inspectedItemObject.id || ('0x' + Number(rawIdVal).toString(16).toUpperCase());
    const uniqueIdVal = inspectedItemObject.uniqueID || 0;
    const enchIdVal = inspectedItemObject.enchantmentID || 0;
    const customNameVal = (inspectedItemObject.customName && inspectedItemObject.customName !== 'Apparel') ? inspectedItemObject.customName : '';

    if (typeof window.AddItemToSlot === 'function' && currentWheelData) {
        window.AddItemToSlot(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: selectedSlotIndex,
            rawID: rawIdVal,
            id: idStrVal,
            uniqueID: uniqueIdVal,
            enchantmentID: enchIdVal,
            customName: customNameVal
        }));
    }

    if (currentWheelData && currentWheelData.slots) {
        const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
        if (slot) {
            if (!slot.items) slot.items = [];
            const alreadyExists = slot.items.some(it => {
                const itRaw = typeof it === 'object' ? (it.rawID || it.formID || parseInt(it.id, 16)) : (typeof it === 'number' ? it : parseInt(it, 16));
                const itUID = typeof it === 'object' ? (it.uniqueID || 0) : 0;
                const itEnch = typeof it === 'object' ? (it.enchantmentID || 0) : 0;
                const itName = typeof it === 'object' ? (it.customName || '') : '';
                return itRaw === rawIdVal && itUID === uniqueIdVal && itEnch === enchIdVal && itName === customNameVal;
            });
            if (!alreadyExists) {
                slot.items.push({
                    name: inspectedItemObject.name,
                    rawID: rawIdVal,
                    id: idStrVal,
                    uniqueID: uniqueIdVal,
                    enchantmentID: enchIdVal,
                    customName: customNameVal,
                    type: inspectedItemObject.type || 'Armor'
                });
            }
            slot.hasOutfit = true;
            slot.count = slot.items.length;
            renderSlotsRail();
            renderDrawerDetails();
            if (typeof updateWheelUI === 'function') updateWheelUI();
        }
    }

    if (typeof showSaveToast === 'function') {
        showSaveToast('Added ' + inspectedItemObject.name + ' to Slot ' + (selectedSlotIndex + 1));
    }
}

function toggleSlotAddonType(isAddon) {
    if (typeof window.SetSlotType === 'function' && currentWheelData) {
        window.SetSlotType(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: selectedSlotIndex,
            isAddon: isAddon
        }));
    }
}

function setSlotRatingDirect(num) {
    if (typeof window.SetSlotRating === 'function' && currentWheelData) {
        window.SetSlotRating(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
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
    slot.contextTag = newTagStr;
    renderDrawerContextChips();
    if (typeof updateWheelUI === 'function') updateWheelUI();

    if (typeof window.SetSlotContext === 'function' && currentWheelData) {
        window.SetSlotContext(JSON.stringify({
            wheelIndex: currentWheelData.wheelIndex || 0,
            slotIndex: selectedSlotIndex,
            contextTag: newTagStr
        }));
    }
}

function saveCurrentEquippedToSlot() {
    saveOutfit(selectedSlotIndex);
}

function equipCurrentSelectedSlot() {
    if (selectedSlotIndex === null || !currentWheelData || !currentWheelData.slots) return;
    const slot = currentWheelData.slots.find(s => (s.index !== undefined ? s.index : 0) === selectedSlotIndex) || currentWheelData.slots[selectedSlotIndex];
    if (slot && slot.hasOutfit && slot.items && slot.items.length > 0) {
        equipOutfit(selectedSlotIndex, true);
    }
}

function clearSelectedSlot() {
    ctxClearSlot();
}

initSortDropdown();
if (typeof document !== 'undefined' && document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initSortDropdown);
}