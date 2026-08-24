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
    autoStashEnabled: false,
    autoSystemEnabled: true,
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
var activeCategoryFilter = 'ALL';
var activeWheelContextFilter = 'ALL';
var activeInventorySort = 'name';
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

var activeDragPanelId = null;
var dragStartX = 0;
var dragStartY = 0;
var panelStartLeft = 0;
var panelStartTop = 0;

var activeResizePanelId = null;
var resizeDirection = '';
var resizeStartX = 0;
var resizeStartY = 0;
var panelStartWidth = 0;
var panelStartHeight = 0;

document.addEventListener('DOMContentLoaded', () => {
    loadWheelPositionFromStorage();
    applyModeLayout();
    loadPanelsLayout();
    initDraggablePanels();
    initResizablePanels();
    initCameraOrbitAndControls();
    initGlobalInputHandlers();
    initCarouselScrollHandlers();
    initWheelCenterInteraction();
    if (typeof updateWheelUI === 'function') updateWheelUI();
});

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

function isAnyModalOpen() {
    const modalIds = ['rename-modal', 'context-modal', 'settings-modal', 'keyboard-modal', 'gamepad-modal'];
    for (let id of modalIds) {
        const el = document.getElementById(id);
        if (el && el.style.display === 'flex') return true;
    }
    const slotCtx = document.getElementById('slot-context-menu');
    const wheelCtx = document.getElementById('wheel-context-menu');
    if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex')) return true;
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

window.openWheel = function(argument) {
    var previousOffsetX = currentWheelData.wheelOffsetX;
    var previousOffsetY = currentWheelData.wheelOffsetY;
    var previousScale = currentWheelData.wheelScale;

    try {
        if (typeof argument === 'string') currentWheelData = JSON.parse(argument);
        else if (typeof argument === 'object' && argument !== null) currentWheelData = argument;
    } catch (e) {}

    if (currentWheelData.wheelOffsetX === undefined || (currentWheelData.wheelOffsetX === 0 && previousOffsetX !== undefined && previousOffsetX !== 0)) {
        currentWheelData.wheelOffsetX = previousOffsetX !== undefined ? previousOffsetX : tempOffsetX;
    }
    if (currentWheelData.wheelOffsetY === undefined || (currentWheelData.wheelOffsetY === 0 && previousOffsetY !== undefined && previousOffsetY !== 0)) {
        currentWheelData.wheelOffsetY = previousOffsetY !== undefined ? previousOffsetY : tempOffsetY;
    }
    if (currentWheelData.wheelScale === undefined || (currentWheelData.wheelScale === 1.0 && previousScale !== undefined && previousScale !== 1.0)) {
        currentWheelData.wheelScale = previousScale !== undefined ? previousScale : tempScale;
    }

    tempOffsetX = currentWheelData.wheelOffsetX || 0.0;
    tempOffsetY = currentWheelData.wheelOffsetY || 0.0;
    tempScale = currentWheelData.wheelScale || 1.0;

    gamepadHoverSlotIndex = null;
    activeWheelContextFilter = 'ALL';
    activeCategoryFilter = 'ALL';

    const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === 'ALL') tab.classList.add('active');
        else tab.classList.remove('active');
    });

    if (typeof hideAllMenus === 'function') hideAllMenus();
    if (typeof hideAllModals === 'function') hideAllModals();
    setTyping(false);
    applyModeLayout();
    loadPanelsLayout();
    renderFollowersStrip();
    renderInventorySourceStrip();
    renderInventoryList();
    renderSlotsRail();
    renderDrawerDetails();
    if (typeof updateWheelUI === 'function') updateWheelUI();
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
                else equipOutfit(realIdx);
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
        if (centerModeText) centerModeText.innerText = "EDIT MODE";
    } else {
        wrapper.classList.add('play-mode-active');
        if (centerModeText) centerModeText.innerText = "PLAY MODE";
    }
    updateGamepadHintsVisibility();
}

function handleWrapperClick(e) {
    if (e.button !== 0) return;
    if (e.target.closest('.context-menu-popup') || e.target.closest('.modal-card') || e.target.closest('[data-slot-index]')) return;

    if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
    if (e.target.id === 'ui-wrapper' || e.target.id === 'wheel-container' || e.target.id === 'wheel-svg' || e.target.id === 'studio-main-stage' || e.target.id === 'panel-viewport') {
        const slotCtx = document.getElementById('slot-context-menu');
        const wheelCtx = document.getElementById('wheel-context-menu');
        if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex')) {
            if (typeof hideAllMenus === 'function') hideAllMenus();
        }
    }
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
        if ((ddEdit && ddEdit.style.display === 'flex') ||
            (ddPlay && ddPlay.style.display === 'flex') ||
            (ddGpPlay && ddGpPlay.style.display === 'flex')) {
            if (typeof closeAllDropdowns === 'function') closeAllDropdowns();
            return true;
        }

        const kbModal = document.getElementById('keyboard-modal');
        if (kbModal && kbModal.style.display === 'flex') {
            if (typeof hideKeyboardModal === 'function') hideKeyboardModal();
            return true;
        }

        const gpModal = document.getElementById('gamepad-modal');
        if (gpModal && gpModal.style.display === 'flex') {
            if (typeof hideGamepadModal === 'function') hideGamepadModal();
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

        const settingsModal = document.getElementById('settings-modal');
        if (settingsModal && settingsModal.style.display === 'flex') {
            if (typeof hideSettingsModal === 'function') hideSettingsModal();
            return true;
        }

        const slotCtx = document.getElementById('slot-context-menu');
        const wheelCtx = document.getElementById('wheel-context-menu');
        if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex')) {
            if (typeof hideAllMenus === 'function') hideAllMenus();
            return true;
        }

        closeWheel();
        return true;
    }
    return false;
}

function initGlobalInputHandlers() {
    window.addEventListener('keydown', handleEscClose, true);

    document.addEventListener('wheel', (e) => {
        if (isEditModeActive()) return;
        if (e.target.closest('#settings-modal') || e.target.closest('#keyboard-modal') || e.target.closest('#gamepad-modal') || e.target.closest('.modal-card') || e.target.closest('.wheel-center')) return;
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

            const code = getScanCode(e);
            if (code) {
                if (listeningKeyType === 'edit') {
                    tempEditKey = code;
                    const btn = document.getElementById('btn-edit-key');
                    if (btn) {
                        btn.innerText = '[' + getKeyName(code) + ']';
                        btn.classList.remove('listening');
                    }
                } else if (listeningKeyType === 'play') {
                    tempPlayKey = code;
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

        if (isTyping) return;

        const pressedScanCode = getScanCode(e);
        const editKey = currentWheelData.editKey || 0x31;
        const editMod = currentWheelData.editModifierKey || 0;
        const playKey = currentWheelData.playKey || 0x2F;
        const playMod = currentWheelData.playModifierKey || 0;

        const shiftHold = e.shiftKey;
        const ctrlHold = e.ctrlKey;
        const altHold = e.altKey;

        const editRequiresMod = (editMod !== 0);
        const playRequiresMod = (playMod !== 0);

        const editModMatch = (editMod === 0) || (editMod === 42 && shiftHold) || (editMod === 29 && ctrlHold) || (editMod === 56 && altHold);
        const playModMatch = (playMod === 0) || (playMod === 42 && shiftHold) || (playMod === 29 && ctrlHold) || (playMod === 56 && altHold);

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
        if (e.button === 2 || e.which === 3) {
            if (e.target.closest('.context-menu-popup') || e.target.closest('.modal-card')) return;

            const slotElem = e.target.closest('[data-slot-index]');
            if (slotElem) {
                const idx = parseInt(slotElem.getAttribute('data-slot-index'));
                if (!isNaN(idx)) {
                    e.preventDefault();
                    e.stopPropagation();
                    const rect = slotElem.getBoundingClientRect();
                    const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (rect.left + rect.width / 2);
                    const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (rect.top + rect.height / 2);
                    if (typeof showSlotContextMenu === 'function') showSlotContextMenu(clickX, clickY, idx);
                    return;
                }
            }

            if (e.target.id === 'ui-wrapper' || e.target.id === 'wheel-container' || e.target.id === 'wheel-svg' || e.target.closest('.wheel-center')) {
                e.preventDefault();
                e.stopPropagation();
                const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (window.innerWidth / 2);
                const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (window.innerHeight / 2);
                if (typeof showWheelContextMenu === 'function') showWheelContextMenu(clickX, clickY);
            }
        }
    }, true);

    window.addEventListener('gamepadconnected', () => { updateGamepadHintsVisibility(); });
    window.addEventListener('gamepaddisconnected', () => { updateGamepadHintsVisibility(); });
}

function initWheelCenterInteraction() {
    document.querySelectorAll('.wheel-center').forEach(centerEl => {
        centerEl.addEventListener('mousedown', (e) => {
            if (e.button === 0) {
                isDraggingWheelCenter = true;
                wheelDragMoved = false;
                wheelDragStartX = e.clientX;
                wheelDragStartY = e.clientY;
                wheelStartOffsetX = (currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0;
                wheelStartOffsetY = (currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0;
                centerEl.classList.add('is-dragging');
                e.stopPropagation();
            }
        });

        centerEl.addEventListener('wheel', (e) => {
            e.preventDefault();
            e.stopPropagation();
            let delta = e.deltaY < 0 ? 0.05 : -0.05;
            let currentScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
            let newScale = Math.max(0.1, currentScale + delta);
            currentWheelData.wheelScale = newScale;
            tempScale = newScale;

            const scaleInp = document.getElementById('scale-input');
            if (scaleInp) scaleInp.value = Math.round(newScale * 100) + '%';

            saveWheelPositionToStorage();
            if (typeof applyTransform === 'function') applyTransform();
            persistWheelSettings();
        }, { passive: false });
    });

    window.addEventListener('mousemove', (e) => {
        if (!isDraggingWheelCenter) return;
        let dx = e.clientX - wheelDragStartX;
        let dy = e.clientY - wheelDragStartY;
        wheelDragMoved = true;

        let newOffsetX = wheelStartOffsetX + dx;
        let newOffsetY = wheelStartOffsetY + dy;
        currentWheelData.wheelOffsetX = newOffsetX;
        currentWheelData.wheelOffsetY = newOffsetY;
        tempOffsetX = newOffsetX;
        tempOffsetY = newOffsetY;

        const offsetInpX = document.getElementById('offsetx-input');
        if (offsetInpX) offsetInpX.value = Math.round(newOffsetX) + 'px';

        const offsetInpY = document.getElementById('offsety-input');
        if (offsetInpY) offsetInpY.value = Math.round(newOffsetY) + 'px';

        if (typeof applyTransform === 'function') applyTransform();
    });

    window.addEventListener('mouseup', () => {
        if (isDraggingWheelCenter) {
            isDraggingWheelCenter = false;
            document.querySelectorAll('.wheel-center').forEach(el => el.classList.remove('is-dragging'));
            if (wheelDragMoved) {
                saveWheelPositionToStorage();
                persistWheelSettings();
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
            editKey: currentWheelData.editKey || 0x31,
            editModifierKey: currentWheelData.editModifierKey || 0,
            playKey: currentWheelData.playKey || 0x2F,
            playModifierKey: currentWheelData.playModifierKey || 0,
            gamepadPlayKey: currentWheelData.gamepadPlayKey || 0x0008,
            gamepadPlayModifier: currentWheelData.gamepadPlayModifier || 0x0100,
            autoStashEnabled: currentWheelData.autoStashEnabled === true,
            autoSystemEnabled: currentWheelData.autoSystemEnabled !== false,
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
            wheelIndex: currentWheelData.wheelIndex,
            slotIndex: slotIndex
        }));
    } else if (typeof window.SaveOutfitToSlot === 'function' && currentWheelData) {
        window.SaveOutfitToSlot(JSON.stringify({
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
            slotIndex: slotIndex
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

function closeWheel() {
    if (typeof hideAllMenus === 'function') hideAllMenus();
    if (typeof hideAllModals === 'function') hideAllModals();
    if (typeof window.OnCloseWheel === 'function') {
        window.OnCloseWheel("");
    }
}

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
    const panelIds = ['panel-inventory', 'panel-slots-rail', 'panel-slot-details'];
    const layout = {};
    panelIds.forEach(id => {
        const el = document.getElementById(id);
        if (el) {
            const rect = el.getBoundingClientRect();
            layout[id] = {
                left: rect.left,
                top: rect.top,
                width: el.offsetWidth,
                height: el.offsetHeight
            };
        }
    });
    try {
        localStorage.setItem('outfit_wheeler_panel_layout', JSON.stringify(layout));
    } catch (err) {}
}

function loadPanelsLayout() {
    try {
        const raw = localStorage.getItem('outfit_wheeler_panel_layout');
        if (!raw) return;
        const layout = JSON.parse(raw);
        const screenW = window.innerWidth;
        const screenH = window.innerHeight;

        Object.keys(layout).forEach(id => {
            const data = layout[id];
            const el = document.getElementById(id);
            if (el && data) {
                let l = Math.max(8, Math.min(data.left, screenW - 100));
                let t = Math.max(8, Math.min(data.top, screenH - 80));
                let w = Math.max(180, Math.min(data.width, screenW - 16));
                let h = Math.max(150, Math.min(data.height, screenH - 16));

                el.style.position = 'fixed';
                el.style.left = l + 'px';
                el.style.top = t + 'px';
                el.style.width = w + 'px';
                el.style.height = h + 'px';
                el.style.right = 'auto';
                el.style.bottom = 'auto';
                el.style.maxWidth = 'none';
                el.style.maxHeight = 'none';
            }
        });
    } catch (err) {}
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
}

function initCameraOrbitAndControls() {
    window.addEventListener('mousedown', (e) => {
        if (!isEditModeActive()) return;
        if (e.button !== 0) return;
        if (e.target.closest('.glass-panel') || e.target.closest('.fullscreen-modal-backdrop') || e.target.closest('#top-follower-bar') || e.target.closest('.context-menu-popup')) return;

        isOrbitingCamera = true;
        lastCamMouseX = e.clientX;
        lastCamMouseY = e.clientY;
        const viewport = document.getElementById('panel-viewport');
        if (viewport) viewport.classList.add('active');
    });

    window.addEventListener('mousemove', (e) => {
        if (!isOrbitingCamera) return;
        const dx = e.clientX - lastCamMouseX;
        const dy = e.clientY - lastCamMouseY;
        lastCamMouseX = e.clientX;
        lastCamMouseY = e.clientY;

        if (typeof window.OrbitCamera === 'function') {
            window.OrbitCamera(JSON.stringify({ deltaX: dx, deltaY: dy }));
        }
    });

    window.addEventListener('mouseup', (e) => {
        if (e.button === 0 && isOrbitingCamera) {
            isOrbitingCamera = false;
            const viewport = document.getElementById('panel-viewport');
            if (viewport) viewport.classList.remove('active');
        }
    });

    window.addEventListener('wheel', (e) => {
        if (!isEditModeActive()) return;
        if (e.target.closest('.panel-body-scrollable') || e.target.closest('.slots-rail-list') || e.target.closest('.drawer-items-list') || e.target.closest('.modal-card') || e.target.closest('#top-follower-bar') || e.target.closest('.inv-source-carousel-wrapper') || e.target.closest('.wheel-center')) return;

        e.preventDefault();
        const delta = Math.sign(e.deltaY);
        if (typeof window.ZoomCamera === 'function') {
            window.ZoomCamera(JSON.stringify({ deltaZoom: delta }));
        }
    }, { passive: false });

    window.addEventListener('keydown', (e) => {
        if (!isEditModeActive()) return;
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
    document.querySelectorAll('.draggable-panel').forEach(panel => {
        panel.addEventListener('mousedown', (e) => {
            if (e.button === 2) {
                e.preventDefault();
                startPanelDrag(e, panel.id);
            }
        });
        panel.addEventListener('contextmenu', (e) => {
            e.preventDefault();
        });
    });

    window.addEventListener('mousemove', (e) => {
        if (!activeDragPanelId) return;
        const panel = document.getElementById(activeDragPanelId);
        if (!panel) return;

        let targetLeft = panelStartLeft + (e.clientX - dragStartX);
        let targetTop = panelStartTop + (e.clientY - dragStartY);

        panel.style.position = 'fixed';
        panel.style.left = targetLeft + 'px';
        panel.style.top = targetTop + 'px';
        panel.style.right = 'auto';
        panel.style.bottom = 'auto';
    });

    window.addEventListener('mouseup', () => {
        if (activeDragPanelId) {
            savePanelsLayout();
            activeDragPanelId = null;
        }
    });
}

function handlePanelDragStart(e, panelId) {
    startPanelDrag(e, panelId);
}

function startPanelDrag(e, panelId) {
    activeDragPanelId = panelId;
    dragStartX = e.clientX;
    dragStartY = e.clientY;
    const panel = document.getElementById(panelId);
    if (panel) {
        const rect = panel.getBoundingClientRect();
        panelStartLeft = rect.left;
        panelStartTop = rect.top;
        panel.style.maxWidth = 'none';
        panel.style.maxHeight = 'none';
    }
}

function initResizablePanels() {
    window.addEventListener('mousemove', (e) => {
        if (!activeResizePanelId) return;
        const panel = document.getElementById(activeResizePanelId);
        if (!panel) return;

        const dx = e.clientX - resizeStartX;
        const dy = e.clientY - resizeStartY;
        const screenW = window.innerWidth;
        const screenH = window.innerHeight;

        let newW = panelStartWidth;
        let newH = panelStartHeight;
        let newLeft = panelStartLeft;
        let newTop = panelStartTop;

        if (resizeDirection.includes('r')) newW = Math.max(180, Math.min(panelStartWidth + dx, screenW - panelStartLeft - 10));
        if (resizeDirection.includes('l')) {
            const maxDx = panelStartWidth - 180;
            const clampedDx = Math.min(maxDx, Math.max(-panelStartLeft + 10, dx));
            newW = panelStartWidth - clampedDx;
            newLeft = panelStartLeft + clampedDx;
        }
        if (resizeDirection.includes('b')) newH = Math.max(150, Math.min(panelStartHeight + dy, screenH - panelStartTop - 10));
        if (resizeDirection.includes('t')) {
            const maxDy = panelStartHeight - 150;
            const clampedDy = Math.min(maxDy, Math.max(-panelStartTop + 10, dy));
            newH = panelStartHeight - clampedDy;
            newTop = panelStartTop + clampedDy;
        }

        panel.style.width = newW + 'px';
        panel.style.height = newH + 'px';
        panel.style.left = newLeft + 'px';
        panel.style.top = newTop + 'px';
    });

    window.addEventListener('mouseup', () => {
        if (activeResizePanelId) {
            savePanelsLayout();
            activeResizePanelId = null;
            resizeDirection = '';
        }
    });
}

function handlePanelResizeStart(e, panelId, dir) {
    if (e.button !== 0) return;
    e.preventDefault();
    e.stopPropagation();

    const panel = document.getElementById(panelId);
    if (!panel) return;

    const rect = panel.getBoundingClientRect();
    panelStartLeft = rect.left;
    panelStartTop = rect.top;
    panelStartWidth = panel.offsetWidth;
    panelStartHeight = panel.offsetHeight;

    panel.style.position = 'fixed';
    panel.style.left = panelStartLeft + 'px';
    panel.style.top = panelStartTop + 'px';
    panel.style.right = 'auto';
    panel.style.bottom = 'auto';
    panel.style.maxWidth = 'none';
    panel.style.maxHeight = 'none';

    activeResizePanelId = panelId;
    resizeDirection = dir;
    resizeStartX = e.clientX;
    resizeStartY = e.clientY;
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
        strip.innerHTML = '<div class="follower-tab-item active" onclick="selectFollower(\'0x00000007\')"><div class="follower-avatar">D</div><span class="follower-name">Player Character</span></div>';
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
    displayList.forEach(f => {
        const fID = (f.id || '').toUpperCase();
        const isActive = (fID === activeTarget);
        const item = document.createElement('div');
        item.className = 'follower-tab-item ' + (isActive ? 'active' : '');
        item.innerHTML = '<div class="follower-avatar">' + (f.avatar || 'F') + '</div><span class="follower-name">' + f.name + '</span>';
        item.onclick = (e) => {
            e.stopPropagation();
            selectFollower(f.id);
        };
        strip.appendChild(item);
    });
}

function selectFollower(targetId) {
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
            sources.push({ id: f.id, type: "FOLLOWER", name: "👤 " + (f.name || 'Follower') });
        }
    });

    sources.push({ id: "CLOUD", type: "CLOUD", name: "☁️ Cloud" });
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

    let displayList = [];
    if (total <= 3) {
        displayList = sources;
    } else {
        const prevIdx = (activeIdx - 1 + total) % total;
        const nextIdx = (activeIdx + 1) % total;
        displayList = [sources[prevIdx], sources[activeIdx], sources[nextIdx]];
    }

    strip.innerHTML = '';
    displayList.forEach(s => {
        const btn = document.createElement('button');
        const isActive = (s.id.toUpperCase() === activeInventorySource.toUpperCase());
        btn.className = 'inv-source-chip ' + (isActive ? 'active' : '');
        btn.innerHTML = '<span>' + s.name + '</span>';
        btn.onclick = (e) => {
            e.stopPropagation();
            setInventorySource(s.id);
        };
        strip.appendChild(btn);
    });
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

function renderInventoryList() {
    const grid = document.getElementById('inventory-items-grid');
    if (!grid) return;
    grid.innerHTML = '';

    let inv = [];
    const srcUpper = activeInventorySource.toUpperCase();

    if (srcUpper === 'CLOUD') {
        inv = currentWheelData.cloudInventory || [];
    } else if (currentWheelData.actorInventories && currentWheelData.actorInventories[activeInventorySource]) {
        inv = currentWheelData.actorInventories[activeInventorySource];
    } else if (srcUpper === '0X00000007' || srcUpper === '0X00000014') {
        inv = currentWheelData.playerInventory || currentWheelData.inventory || [];
    } else {
        inv = currentWheelData.followerInventory || [];
    }

    let items = inv.filter(item => {
        if (activeCategoryFilter === 'ARMORS' && item.category !== 'ARMORS') return false;
        if (activeCategoryFilter === 'JEWELRY' && item.category !== 'JEWELRY') return false;
        if (inventorySearchQuery) {
            return (item.name || '').toLowerCase().includes(inventorySearchQuery) || (item.type || '').toLowerCase().includes(inventorySearchQuery);
        }
        return true;
    });

    items.sort((a, b) => {
        if (activeInventorySort === 'name') return (a.name || '').localeCompare(b.name || '');
        if (activeInventorySort === 'rating') return (b.armor || 0) - (a.armor || 0);
        if (activeInventorySort === 'weight') return (a.weight || 0) - (b.weight || 0);
        if (activeInventorySort === 'value') return (b.value || 0) - (a.value || 0);
        return 0;
    });

    const cAll = document.getElementById('count-all');
    const cArm = document.getElementById('count-armors');
    const cJwl = document.getElementById('count-jewelry');

    if (cAll) cAll.innerText = inv.length;
    if (cArm) cArm.innerText = inv.filter(x => x.category === 'ARMORS').length;
    if (cJwl) cJwl.innerText = inv.filter(x => x.category === 'JEWELRY').length;

    if (items.length === 0) {
        let emptyMsg = 'No apparel items found.';
        if (srcUpper === 'CLOUD') emptyMsg = 'Cloud Wardrobe Chest is empty.';
        grid.innerHTML = '<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">' + emptyMsg + '</div>';
        return;
    }

    items.forEach(item => {
        const card = document.createElement('div');
        card.className = 'item-row-card ' + (item.equipped ? 'is-equipped-item' : '');

        let iconClass = 'icon-armor';
        if (item.category === 'JEWELRY') iconClass = 'icon-jewelry';

        let badgeTagsHtml = '';
        if (item.source === 'CLOUD') badgeTagsHtml += '<span class="badge-tag" style="background:rgba(82,165,245,0.2); color:#52a5f5; border:1px solid #52a5f5;">CLOUD</span>';
        if (item.tags && item.tags.includes('enchanted')) badgeTagsHtml += '<span class="badge-tag tag-enchanted">ENCH</span>';
        if (item.tags && item.tags.includes('stolen')) badgeTagsHtml += '<span class="badge-tag tag-stolen">STOLEN</span>';

        let statPillText = 'ARM ' + (item.armor || 0);

        card.innerHTML = '<div class="item-left-info"><div class="item-icon-box ' + iconClass + '">' + (item.icon || '🛡️') + '</div><div class="item-meta-titles"><span class="item-name-text">' + item.name + '</span><span class="item-sub-desc">' + (item.type || 'Apparel') + ' • Wt ' + Number(item.weight || 0).toFixed(1) + ' • ' + (item.value || 0) + 'g</span></div></div><div class="item-right-stats"><div class="item-badges-strip">' + badgeTagsHtml + '</div><span class="item-stat-pill">' + statPillText + '</span></div>';

        card.onpointerenter = () => inspectItem(item);
        card.onclick = () => {
            if (typeof window.ToggleItemEquip === 'function') {
                window.ToggleItemEquip(JSON.stringify({ rawID: item.rawID, id: item.id, source: item.source || activeInventorySource }));
            }
        };

        grid.appendChild(card);
    });
}

function inspectItem(item) {
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
    if (!rail || !currentWheelData || !currentWheelData.slots) return;
    rail.innerHTML = '';

    const slots = currentWheelData.slots;
    slots.forEach((s, idx) => {
        const realIdx = s.index !== undefined ? s.index : idx;
        const card = document.createElement('div');
        const isSelected = (realIdx === selectedSlotIndex);
        card.className = 'vertical-slot-card ' + (isSelected ? 'active' : '') + ' ' + (s.isEquipped ? 'is-equipped' : '') + ' ' + (s.isAddon ? 'is-addon' : '');

        let pipHtml = '';
        if (s.isAddon) pipHtml = '<span class="slot-status-pip pip-addon">ADDON</span>';
        else if (s.isEquipped) pipHtml = '<span class="slot-status-pip pip-equipped">EQUIPPED</span>';

        let icon = s.isAddon ? '✨' : (s.hasOutfit ? '🛡️' : '➕');

        card.innerHTML = '<div class="slot-card-icon">' + icon + '</div><div class="slot-card-texts"><span class="slot-card-name">' + (s.hasOutfit ? s.name : 'Empty Slot') + '</span><span class="slot-card-sub">Slot ' + (realIdx + 1) + ' • ' + (s.count || 0) + ' pcs</span></div>' + pipHtml;

        card.onclick = () => {
            selectedSlotIndex = realIdx;
            renderSlotsRail();
            renderDrawerDetails();
            if (s.hasOutfit) {
                equipOutfit(realIdx);
            }
        };

        rail.appendChild(card);
    });
}