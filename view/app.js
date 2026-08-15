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
    wheelScale: 1.0,
    wheelOffsetX: 0.0,
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
    slots: []
};

var activeCategoryFilter = 'ALL';
var lastEquippedSlotIndex = null;
var selectedSlotIndex = null;
var gamepadHoverSlotIndex = null;
var renameTargetMode = 'slot';
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
var tempStars = 3;
var toastTimeout = null;
var lastMenuOpenTime = 0;

var modNameMap = { 0: 'None', 42: 'Shift', 29: 'Ctrl', 56: 'Alt' };
var gpModNameMap = { 0: 'None', 256: 'LB', 512: 'RB', 9: 'LT', 10: 'RT' };

var svgOutfitIcon = '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M7 2L2 5V10C2 15.52 6.48 20.74 12 22C17.52 20.74 22 15.52 22 10V5L17 2L12 5L7 2Z" fill="rgba(82, 165, 245, 0.15)" stroke="#79bac1" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/><path d="M12 5V22" stroke="#6c829c" stroke-width="1.2" stroke-dasharray="2 2"/><path d="M7 8H17" stroke="#79bac1" stroke-width="1.5" stroke-linecap="round"/><path d="M8 12H16" stroke="#6c829c" stroke-width="1.2" stroke-linecap="round"/><circle cx="12" cy="8" r="1.5" fill="#fad165"/></svg>';
var svgPlusIcon = '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><circle cx="12" cy="12" r="9" stroke="#6c829c" stroke-width="1.5" stroke-dasharray="3 3"/><path d="M12 8V16M8 12H16" stroke="#a3b8cc" stroke-width="2" stroke-linecap="round"/></svg>';

function getScanCode(e) {
    if (e.code && scanCodeMap[e.code]) {
        return scanCodeMap[e.code];
    }
    const code = e.keyCode || e.which;
    if (code >= 112 && code <= 123) {
        const fKeyMap = {
            112: 0x3B, 113: 0x3C, 114: 0x3D, 115: 0x3E, 116: 0x3F, 117: 0x40,
            118: 0x41, 119: 0x42, 120: 0x43, 121: 0x44, 122: 0x57, 123: 0x58
        };
        return fKeyMap[code] || 0x3B;
    }
    if (code >= 65 && code <= 90) {
        const letterMap = {
            65: 0x1E, 66: 0x30, 67: 0x2E, 68: 0x20, 69: 0x12, 70: 0x21, 71: 0x22,
            72: 0x23, 73: 0x17, 74: 0x24, 75: 0x25, 76: 0x26, 77: 0x32, 78: 0x31,
            79: 0x18, 80: 0x19, 81: 0x10, 82: 0x13, 83: 0x1F, 84: 0x14, 85: 0x16,
            86: 0x2F, 87: 0x11, 88: 0x2D, 89: 0x15, 90: 0x2C
        };
        return letterMap[code] || 0x31;
    }
    if (code >= 48 && code <= 57) {
        const digitMap = {
            48: 0x0B, 49: 0x02, 50: 0x03, 51: 0x04, 52: 0x05,
            53: 0x06, 54: 0x07, 55: 0x08, 56: 0x09, 57: 0x0A
        };
        return digitMap[code] || 0x02;
    }
    const specialMap = {
        8: 0x0E, 9: 0x0F, 13: 0x1C, 16: 0x2A, 17: 0x1D, 18: 0x38, 20: 0x3A, 27: 0x01, 32: 0x39,
        33: 0xC9, 34: 0xD1, 35: 0xCF, 36: 0xC7, 37: 0xC8, 39: 0xCD, 40: 0xD0,
        45: 0xD2, 46: 0xD3, 186: 0x27, 187: 0x0D, 188: 0x33, 189: 0x0C, 190: 0x34,
        191: 0x35, 192: 0x29, 219: 0x1A, 220: 0x2B, 221: 0x1B, 222: 0x28
    };
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
        if (scanCodeMap[key] === dxCode) {
            return key.replace('Key', '').replace('Digit', '').replace('Arrow', '');
        }
    }
    return '0x' + dxCode.toString(16).toUpperCase();
}

function isEditModeActive() {
    if (!currentWheelData) return false;
    return (currentWheelData.editMode === true || currentWheelData.editMode === "true" || currentWheelData.editMode === 1);
}

function updateGamepadHintsVisibility() {
    const hintsBar = document.getElementById('gamepad-hints-bar');
    if (!hintsBar) return;

    if (isEditModeActive()) {
        hintsBar.style.display = 'none';
        return;
    }

    let hasGp = false;
    if (currentWheelData && currentWheelData.gamepadConnected !== undefined) {
        hasGp = (currentWheelData.gamepadConnected === true || currentWheelData.gamepadConnected === "true" || currentWheelData.gamepadConnected === 1);
    } else if (typeof navigator.getGamepads === 'function') {
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
    try {
        if (typeof argument === 'string') {
            currentWheelData = JSON.parse(argument);
        } else if (typeof argument === 'object' && argument !== null) {
            currentWheelData = argument;
        }
    } catch (e) {}

    gamepadHoverSlotIndex = null;
    activeCategoryFilter = 'ALL';
    const tabs = document.querySelectorAll('.filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === 'ALL') {
            tab.classList.add('active');
        } else {
            tab.classList.remove('active');
        }
    });

    hideAllMenus();
    hideAllModals();
    setTyping(false);
    updateWheelUI();
};

window.setGamepadAngle = function(phiStr) {
    const phi = parseFloat(phiStr);
    if (isNaN(phi)) return;

    const paths = document.querySelectorAll('.slice-path');
    const total = paths.length;
    if (total === 0) return;

    const sliceAngle = (2 * Math.PI) / total;
    let targetIdx = Math.floor(phi / sliceAngle);
    if (targetIdx >= total) targetIdx = total - 1;

    if (targetIdx !== gamepadHoverSlotIndex) {
        gamepadHoverSlotIndex = targetIdx;
        paths.forEach((p, idx) => {
            if (idx === targetIdx) {
                p.classList.add('gamepad-hover');
            } else {
                p.classList.remove('gamepad-hover');
            }
        });
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
    const paths = document.querySelectorAll('.slice-path');
    if (gamepadHoverSlotIndex !== null && gamepadHoverSlotIndex < paths.length) {
        const targetPath = paths[gamepadHoverSlotIndex];
        if (targetPath) {
            const realIdx = parseInt(targetPath.getAttribute('data-slot-index'));
            if (!isNaN(realIdx)) {
                if (isEditModeActive()) {
                    saveOutfit(realIdx);
                } else {
                    equipOutfit(realIdx);
                }
            }
        }
    }
};

window.gamepadCycleWheel = function(deltaStr) {
    const delta = parseInt(deltaStr) || 1;
    changeWheel(delta);
};

window.gamepadCycleFilter = function(deltaStr) {
    const delta = parseInt(deltaStr) || 1;
    const tags = ['ALL', 'City', 'Home', 'Combat', 'Swim'];
    let currentIdx = tags.indexOf(activeCategoryFilter);
    if (currentIdx === -1) currentIdx = 0;
    currentIdx = (currentIdx + delta + tags.length) % tags.length;
    setCategoryFilter(tags[currentIdx]);
};

function setTyping(state) {
    isTyping = state;
    if (typeof window.SetTypingMode === 'function') {
        window.SetTypingMode(JSON.stringify({ typing: state }));
    }
}

function startKeyListen(type) {
    listeningKeyType = type;
    listeningGamepadType = null;
    const btn = document.getElementById(type === 'edit' ? 'btn-edit-key' : 'btn-play-key');
    if (btn) {
        btn.innerText = "Press key...";
        btn.classList.add('listening');
        btn.blur();
    }
}

function startGamepadListen(type) {
    listeningGamepadType = type;
    listeningKeyType = null;
    const btn = document.getElementById('btn-gp-play-key');
    if (btn) {
        btn.innerText = "Press button...";
        btn.classList.add('listening');
        btn.blur();
    }
}

function handleWrapperClick(e) {
    if (Date.now() - lastMenuOpenTime < 300) {
        return;
    }
    if (e.button !== 0) return;
    if (e.target.closest('.context-menu-popup') || e.target.closest('.modal-card') || e.target.closest('[data-slot-index]')) {
        return;
    }

    closeAllDropdowns();
    if (e.target.id === 'ui-wrapper' || e.target.id === 'wheel-container' || e.target.id === 'wheel-svg') {
        const slotCtx = document.getElementById('slot-context-menu');
        const wheelCtx = document.getElementById('wheel-context-menu');
        if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex')) {
            hideAllMenus();
            return;
        }
    }
}

document.addEventListener('mousedown', (e) => {
    if (e.button === 2 || e.which === 3) {
        if (e.target.closest('.context-menu-popup') || e.target.closest('.modal-card')) {
            return;
        }

        const slotElem = e.target.closest('[data-slot-index]');
        if (slotElem) {
            const idx = parseInt(slotElem.getAttribute('data-slot-index'));
            if (!isNaN(idx)) {
                e.preventDefault();
                e.stopPropagation();
                const rect = slotElem.getBoundingClientRect();
                const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (rect.left + rect.width / 2);
                const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (rect.top + rect.height / 2);
                showSlotContextMenu(clickX, clickY, idx);
                return;
            }
        }

        if (e.target.id === 'ui-wrapper' || e.target.id === 'wheel-container' || e.target.id === 'wheel-svg' || e.target.closest('.wheel-center')) {
            e.preventDefault();
            e.stopPropagation();
            const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : (window.innerWidth / 2);
            const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : (window.innerHeight / 2);
            showWheelContextMenu(clickX, clickY);
        }
    }
}, true);

function handleEscClose(e) {
    if (e.key === 'Escape' || e.code === 'Escape' || e.keyCode === 27) {
        if (e) {
            e.preventDefault();
            e.stopPropagation();
        }

        const renameModal = document.getElementById('rename-modal');
        if (renameModal && renameModal.style.display === 'flex') {
            hideRenameModal();
            return true;
        }
        const ctxModal = document.getElementById('context-modal');
        if (ctxModal && ctxModal.style.display === 'flex') {
            hideContextModal();
            return true;
        }
        const gpModal = document.getElementById('gamepad-modal');
        if (gpModal && gpModal.style.display === 'flex') {
            hideGamepadModal();
            return true;
        }
        const kbModal = document.getElementById('keyboard-modal');
        if (kbModal && kbModal.style.display === 'flex') {
            hideKeyboardModal();
            return true;
        }
        const settingsModal = document.getElementById('settings-modal');
        if (settingsModal && settingsModal.style.display === 'flex') {
            hideSettingsModal();
            return true;
        }
        const slotCtx = document.getElementById('slot-context-menu');
        const wheelCtx = document.getElementById('wheel-context-menu');
        if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex')) {
            hideAllMenus();
            return true;
        }
        closeWheel();
        return true;
    }
    return false;
}

window.addEventListener('keydown', handleEscClose, true);
window.addEventListener('keyup', handleEscClose, true);

document.addEventListener('wheel', (e) => {
    if (e.target.closest('#settings-modal') || e.target.closest('#keyboard-modal') || e.target.closest('#gamepad-modal') || e.target.closest('.modal-card')) {
        return;
    }
    if (currentWheelData) {
        const delta = e.deltaY < 0 ? -1 : 1;
        changeWheel(delta);
    }
});

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

window.addEventListener('gamepadconnected', () => {
    updateGamepadHintsVisibility();
});

window.addEventListener('gamepaddisconnected', () => {
    updateGamepadHintsVisibility();
});

document.addEventListener('DOMContentLoaded', () => {
    updateWheelUI();
});