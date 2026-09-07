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
    if (typeof updateWheelUI === 'function') updateWheelUI();
});

window.addEventListener('resize', () => {
    applyAutoResolutionScale();
    loadPanelsLayout();
    if (typeof updateWheelUI === 'function') updateWheelUI();
});

function getUiScale() {
    if (currentWheelData && currentWheelData.uiScale !== undefined) {
        const s = parseFloat(currentWheelData.uiScale);
        if (!isNaN(s) && s >= 0.5 && s <= 3.0) return s;
    }
    const stored = parseFloat(localStorage.getItem('outfit_wheeler_ui_scale'));
    if (!isNaN(stored) && stored >= 0.5 && stored <= 3.0) return stored;
    return 1.0;
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
    if (typeof window.SetTypingMode === 'function') {
        window.SetTypingMode(JSON.stringify({ typing: state }));
    }
}

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
    const menus = ['slot-context-menu', 'wheel-context-menu', 'item-context-menu', 'transfer-all-context-menu'];
    menus.forEach(id => {
        const el = document.getElementById(id);
        if (el) el.style.display = 'none';
    });
    if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
}

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
    hintsBar.style.display = hasGp ? 'flex' : 'none';
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
        if (currentWheelData.uiScale === undefined || currentWheelData.uiScale === 1.0) {
            currentWheelData.uiScale = storedUiScale;
        } else {
            safeSetStorage('outfit_wheeler_ui_scale', String(currentWheelData.uiScale));
        }
    } else if (currentWheelData.uiScale !== undefined) {
        safeSetStorage('outfit_wheeler_ui_scale', String(currentWheelData.uiScale));
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
                target.classList.contains('dropdown-list')) {
                return;
            }
        }
        if (target.id === 'panel-settings') return;
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
        if (safeClosest(e.target, '#panel-settings') || safeClosest(e.target, '.modal-card') || safeClosest(e.target, '#panel-inventory') || safeClosest(e.target, '#panel-drawer') || safeClosest(e.target, '.panel-body-scrollable') || safeClosest(e.target, '.custom-sort-list')) return;
        if (isDraggingWheelCenter && wheelDragButton === 0) return;
        if (isEditModeActive() && !(typeof isClassicEditMode === 'function' && isClassicEditMode()) && !safeClosest(e.target, '#wheel-container')) return;
        const now = Date.now();
        if (now - lastWheelScrollTimestamp < 150) return;
        lastWheelScrollTimestamp = now;

        if (currentWheelData) {
            const delta = e.deltaY < 0 ? -1 : 1;
            changeWheel(delta);
        }
    }, { passive: false });

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

        const pressedScanCode = getScanCode(e);
        const editKey = currentWheelData.editKey || 0x31;
        const editMod = currentWheelData.editModifierKey !== undefined ? (parseInt(currentWheelData.editModifierKey, 10) || 0) : 0;
        const playKey = currentWheelData.playKey || 0x2F;
        const playMod = currentWheelData.playModifierKey !== undefined ? (parseInt(currentWheelData.playModifierKey, 10) || 0) : 0;

        const shiftHold = e.shiftKey;
        const ctrlHold = e.ctrlKey;
        const altHold = e.altKey;

        function checkModMatch(mod) {
            if (mod === 0) return !shiftHold && !ctrlHold && !altHold;
            if ((mod === 42 || mod === 54 || mod === 0x2A || mod === 0x36) && shiftHold) return true;
            if ((mod === 29 || mod === 157 || mod === 0x1D || mod === 0x9D) && ctrlHold) return true;
            if ((mod === 56 || mod === 184 || mod === 0x38 || mod === 0xB8) && altHold) return true;
            return false;
        }

        const editModMatch = checkModMatch(editMod);
        const playModMatch = checkModMatch(playMod);
        const editRequiresMod = (editMod !== 0);
        const playRequiresMod = (playMod !== 0);

        let matchEdit = (pressedScanCode === editKey && editModMatch);
        let matchPlay = (pressedScanCode === playKey && playModMatch);

        if (editRequiresMod && !editModMatch) matchEdit = false;
        if (playRequiresMod && !playModMatch) matchPlay = false;

        if (editRequiresMod && matchEdit) matchPlay = false;
        else if (playRequiresMod && matchPlay) matchEdit = false;

        if (matchEdit || matchPlay) {
            e.preventDefault();
            e.stopPropagation();
            closeWheel();
        }
    });

    document.addEventListener('mousedown', (e) => {
        const isInsideMenu = !!safeClosest(e.target, '.context-menu-popup');
        const isInsideDropdown = !!safeClosest(e.target, '.custom-dropdown, .custom-sort-dropdown, .dropdown-list, .custom-sort-list, #panel-settings, .modifier-pill-group');

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

            const itemElem = safeClosest(e.target, '.item-row-card');
            if (itemElem) {
                e.preventDefault();
                e.stopPropagation();

                const itemMenu = document.getElementById('item-context-menu');
                const itemNameEl = itemElem.querySelector('.item-name-text');
                const itemName = itemNameEl ? itemNameEl.innerText.trim() : '';

                const inv = getInventoryBySourceId(activeInventorySource);
                const matchedItem = inv.find(x => x.name === itemName) || inspectedItemObject;

                if (itemMenu && itemMenu.style.display === 'flex' && inspectedItemObject && (inspectedItemObject.name === itemName || (matchedItem && inspectedItemObject.rawID === matchedItem.rawID))) {
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

        const dx = e.clientX - wheelDragStartX;
        const dy = e.clientY - wheelDragStartY;
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
    if (typeof window.AddSlot === 'function' && currentWheelData) {
        window.AddSlot(JSON.stringify({
            editMode: isEditModeActive(),
            contextTag: activeWheelContextFilter && activeWheelContextFilter !== 'ALL' ? activeWheelContextFilter : ""
        }));
    }
}

function closeWheel() {
    const wrapper = document.getElementById('ui-wrapper');
    if (wrapper) {
        wrapper.classList.remove('is-open');
        wrapper.style.display = 'none';
        wrapper.style.pointerEvents = 'none';
    }
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
    const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
    panelIds.forEach(id => {
        const el = document.getElementById(id);
        if (el && el.offsetWidth > 100 && el.offsetHeight > 100) {
            const rect = el.getBoundingClientRect();
            if (rect.width > 100 && rect.height > 100) {
                const w = Math.round(rect.width / scale);
                const h = Math.round(rect.height / scale);
                const l = Math.round(rect.left / scale);
                const t = Math.round(rect.top / scale);

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
        const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
        const screenW = Math.round(window.innerWidth / scale);
        const screenH = Math.round(window.innerHeight / scale);

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
            e.preventDefault();
            e.stopPropagation();
            const now = Date.now();
            if (now - lastFollowerScrollTimestamp < 140) return;
            lastFollowerScrollTimestamp = now;
            cycleFollowerCarousel(e.deltaY < 0 ? -1 : 1);
        }, { passive: false });
    }

    const srcWrapper = document.getElementById('inv-source-carousel-wrapper');
    if (srcWrapper) {
        srcWrapper.addEventListener('wheel', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const now = Date.now();
            if (now - lastSourceScrollTimestamp < 140) return;
            lastSourceScrollTimestamp = now;
            cycleInvSource(e.deltaY < 0 ? -1 : 1);
        }, { passive: false });
    }

    const sortList = document.getElementById('custom-sort-list');
    if (sortList) {
        sortList.addEventListener('wheel', (e) => {
            e.stopPropagation();
        }, { passive: true });
    }
}

function initCameraOrbitAndControls() {
    window.addEventListener('mousedown', (e) => {
        if (!isEditModeActive()) return;
        if (typeof isClassicEditMode === 'function' && isClassicEditMode()) return;
        if (e.button !== 0) return;
        if (safeClosest(e.target, '.glass-panel') || safeClosest(e.target, '.fullscreen-modal-backdrop') || safeClosest(e.target, '#top-follower-bar') || safeClosest(e.target, '.context-menu-popup')) return;

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

                const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
                const screenW = Math.round(window.innerWidth / scale);
                const screenH = Math.round(window.innerHeight / scale);
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
                    const oLeft = oRect.left / scale;
                    const oRight = oRect.right / scale;
                    const oTop = oRect.top / scale;
                    const oBottom = oRect.bottom / scale;

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
        const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
        const rect = panel.getBoundingClientRect();
        panelStartLeft = rect.left / scale;
        panelStartTop = rect.top / scale;
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

            const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
            const dx = (resizeCurrentClientX - resizeStartX) / scale;
            const dy = (resizeCurrentClientY - resizeStartY) / scale;
            const screenW = Math.round(window.innerWidth / scale);
            const screenH = Math.round(window.innerHeight / scale);

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

    const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
    const rect = panel.getBoundingClientRect();
    panelStartLeft = rect.left / scale;
    panelStartTop = rect.top / scale;
    panelStartWidth = rect.width / scale;
    panelStartHeight = rect.height / scale;

    panel.style.position = 'fixed';
    panel.style.left = Math.round(panelStartLeft) + 'px';
    panel.style.top = Math.round(panelStartTop) + 'px';
    panel.style.width = Math.round(panelStartWidth) + 'px';
    panel.style.height = Math.round(panelStartHeight) + 'px';
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
                const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
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
                const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
                const pointX = (scale !== 1.0) ? (e.clientX / scale) : e.clientX;
                const pointY = (scale !== 1.0) ? (e.clientY / scale) : e.clientY;
                dragGhostElement.style.left = (pointX + 12) + 'px';
                dragGhostElement.style.top = (pointY + 12) + 'px';

                document.querySelectorAll('.drag-hover-target').forEach(el => el.classList.remove('drag-hover-target'));

                const elemBelow = document.elementFromPoint(pointX, pointY);
                if (elemBelow) {
                    const targetSlotCard = safeClosest(elemBelow, '.vertical-slot-card');
                    const targetSlicePath = safeClosest(elemBelow, '.slice-path');
                    const targetLabel = safeClosest(elemBelow, '.wheel-slot-label');
                    const targetInspector = safeClosest(elemBelow, '#panel-slot-details');

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
                const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
                const pointX = (scale !== 1.0) ? (e.clientX / scale) : e.clientX;
                const pointY = (scale !== 1.0) ? (e.clientY / scale) : e.clientY;
                slotDragGhostElement.style.left = (pointX + 12) + 'px';
                slotDragGhostElement.style.top = (pointY + 12) + 'px';

                document.querySelectorAll('.slot-drop-hover').forEach(el => el.classList.remove('slot-drop-hover'));

                const elemBelow = document.elementFromPoint(pointX, pointY);
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
                const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
                const pointX = (scale !== 1.0) ? (e.clientX / scale) : e.clientX;
                const pointY = (scale !== 1.0) ? (e.clientY / scale) : e.clientY;
                const elemBelow = document.elementFromPoint(pointX, pointY);
                if (elemBelow) {
                    const targetSlotCard = safeClosest(elemBelow, '.vertical-slot-card');
                    const targetSlicePath = safeClosest(elemBelow, '.slice-path');
                    const targetLabel = safeClosest(elemBelow, '.wheel-slot-label');
                    const targetInspector = safeClosest(elemBelow, '#panel-slot-details');

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
                const scale = (typeof getUiScale === 'function' ? getUiScale() : 1) || 1;
                const pointX = (scale !== 1.0) ? (e.clientX / scale) : e.clientX;
                const pointY = (scale !== 1.0) ? (e.clientY / scale) : e.clientY;
                const elemBelow = document.elementFromPoint(pointX, pointY);
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
    if (typeof window.SelectTargetActor === 'function') {
        window.SelectTargetActor(JSON.stringify({ id: targetId }));
    }
}

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
    activeInventorySource = sourceId;
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

function renderInventoryList() {
    const grid = document.getElementById('inventory-items-grid');
    if (!grid) return;
    grid.innerHTML = '';

    const inv = getInventoryBySourceId(activeInventorySource);
    const srcUpper = activeInventorySource.toUpperCase();

    let items = inv.filter(item => {
        if (!item.name || item.name.trim().length === 0) return false;
        if (activeCategoryFilter === 'ARMORS' && item.category !== 'ARMORS') return false;
        if (activeCategoryFilter === 'JEWELRY' && item.category !== 'JEWELRY') return false;
        if (inventorySearchQuery) {
            return (item.name || '').toLowerCase().includes(inventorySearchQuery) || (item.type || '').toLowerCase().includes(inventorySearchQuery);
        }
        return true;
    });

    items.sort((a, b) => {
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

    const cAll = document.getElementById('count-all');
    const cArm = document.getElementById('count-armors');
    const cJwl = document.getElementById('count-jewelry');

    if (cAll) cAll.innerText = inv.length;
    if (cArm) cArm.innerText = inv.filter(x => x.category === 'ARMORS').length;
    if (cJwl) cJwl.innerText = inv.filter(x => x.category === 'JEWELRY').length;

    const statWeight = document.getElementById('stat-carry-weight');
    if (statWeight) {
        if (srcUpper === 'CLOUD' || srcUpper === 'WARDROBE') {
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

    if (items.length === 0) {
        let emptyMsg = 'No apparel items found.';
        if (srcUpper === 'CLOUD') emptyMsg = 'Wardrobe Chest is empty.';
        grid.innerHTML = '<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">' + emptyMsg + '</div>';
        return;
    }

    const fragment = document.createDocumentFragment();

    items.forEach(item => {
        const card = document.createElement('div');
        card.className = 'item-row-card ' + (item.equipped ? 'is-equipped-item' : '');

        let badgeTagsHtml = '';
        if (item.equipped) badgeTagsHtml += '<span class="badge-tag tag-equipped" style="background:rgba(16,185,129,0.2); color:#10b981; border:1px solid #10b981; font-weight:bold;">EQUIPPED</span>';
        if (item.source === 'CLOUD' || srcUpper === 'CLOUD') badgeTagsHtml += '<span class="badge-tag" style="background:rgba(82,165,245,0.2); color:#52a5f5; border:1px solid #52a5f5;">WARDROBE</span>';
        if (item.tags && item.tags.includes('enchanted')) badgeTagsHtml += '<span class="badge-tag tag-enchanted">ENCH</span>';
        if (item.tags && item.tags.includes('stolen')) badgeTagsHtml += '<span class="badge-tag tag-stolen">STOLEN</span>';

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
            if (typeof window.ToggleItemEquip === 'function') {
                window.ToggleItemEquip(JSON.stringify({ rawID: item.rawID, id: item.id, source: item.source || activeInventorySource }));
            }
        };

        fragment.appendChild(card);
    });
    grid.appendChild(fragment);
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

    const currentItemSource = (activeInventorySource || item.source || '0x00000007').toUpperCase();
    const allSources = getInventorySourcesList();

    allSources.forEach(src => {
        const srcUpper = src.id.toUpperCase();
        if (srcUpper !== currentItemSource) {
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

    let posX = (typeof x === 'number' && x > 0) ? x : (window.innerWidth / 2);
    let posY = (typeof y === 'number' && y > 0) ? y : (window.innerHeight / 2);

    posX = Math.max(10, Math.min(posX, window.innerWidth - 220));
    posY = Math.max(10, Math.min(posY, window.innerHeight - 300));

    menu.style.left = posX + 'px';
    menu.style.top = posY + 'px';
    menu.style.display = 'flex';
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
        if (src.id.toUpperCase() !== currentSource) {
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
        posY = rect.top - 120;
    }

    posX = Math.max(10, Math.min(posX, window.innerWidth - 220));
    posY = Math.max(10, Math.min(posY, window.innerHeight - 200));

    menu.style.left = posX + 'px';
    menu.style.top = posY + 'px';
    menu.style.display = 'flex';
}

function performLocalItemTransfer(rawIdVal, fromSrc, toTargetId) {
    const srcList = getInventoryBySourceId(fromSrc);
    const dstList = getInventoryBySourceId(toTargetId);

    if (Array.isArray(srcList) && Array.isArray(dstList) && srcList !== dstList) {
        const idx = srcList.findIndex(it => (it.rawID || parseInt(it.id, 16)) === rawIdVal);
        if (idx !== -1) {
            const [item] = srcList.splice(idx, 1);
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

    performLocalItemTransfer(rawIdVal, fromSrc, toTargetId);

    if (typeof window.TransferItem === 'function') {
        window.TransferItem(JSON.stringify({
            rawID: rawIdVal,
            id: idStrVal,
            fromSource: fromSrc,
            toTarget: toTargetId
        }));
    }

    showSaveToast("Transferred " + inspectedItemObject.name);
    hideAllMenus();
}

function inspectItem(item) {
    inspectedItemObject = item;
    const tag = document.getElementById('inspect-type-tag');
    const title = document.getElementById('inspect-title-text');
    const priLbl = document.getElementById('stat-primary-lbl');
    const priVal = document.getElementById('stat-primary-val');
    const wtVal = document.getElementById('stat-weight-val');
    const valVal = document.getElementById('stat-value-val');
    const enchBox = document.getElementById('inspect-enchantment-box');
    const enchDesc = document.getElementById('inspect-ench-desc');

    if (!tag || !title || !priLbl || !priVal) return;

    tag.innerText = item.category || 'ARMOR';
    title.innerText = item.name;

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