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
    ['rename-modal', 'context-modal', 'color-picker-modal'].forEach(id => {
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
        if (posY + menuH > vh - 15 || posY > vh / 2) {
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

/* ==========================================================================
   DYNAMIC THEME ENGINE (From theme_preview.html)
   ========================================================================== */
var GEM_PRESETS = [
    { id: 'sapphire', name: 'Sapphire Archmage', hex: '#38bdf8', bgHex: '#070b14', textHex: '#f0f9ff', depth: 0.60 },
    { id: 'ruby', name: 'Ruby Daedric', hex: '#ef4444', bgHex: '#180608', textHex: '#fff1f2', depth: 0.65 },
    { id: 'amethyst', name: 'Amethyst Mystic', hex: '#a855f7', bgHex: '#12071f', textHex: '#faf5ff', depth: 0.62 },
    { id: 'emerald', name: 'Emerald Poison', hex: '#10b981', bgHex: '#05190e', textHex: '#ecfdf5', depth: 0.58 },
    { id: 'amber', name: 'Nordic Amber', hex: '#f59e0b', bgHex: '#180e05', textHex: '#fefce8', depth: 0.60 },
    { id: 'cyan', name: 'Cyber Cyan', hex: '#06b6d4', bgHex: '#061318', textHex: '#ecfeff', depth: 0.55 },
    { id: 'rose', name: 'Sanguine Rose', hex: '#f43f5e', bgHex: '#18060d', textHex: '#fff1f2', depth: 0.65 },
    { id: 'silver', name: 'OLED Platinum', hex: '#94a3b8', bgHex: '#000000', textHex: '#ffffff', depth: 0.10 },
    { id: 'parchment', name: 'Elder Parchment', hex: '#d97706', bgHex: '#1f1912', textHex: '#fef3c7', depth: 0.50 },
    { id: 'graphite', name: 'Midnight Graphite', hex: '#38bdf8', bgHex: '#111827', textHex: '#f9fafb', depth: 0.40 }
];

var currentThemeHex = safeGetStorage('outfit_wheeler_theme_hex', '#38bdf8') || '#38bdf8';
var currentBgHex = safeGetStorage('outfit_wheeler_theme_bg', '#070a0f') || '#070a0f';
var currentTextHex = safeGetStorage('outfit_wheeler_theme_text', '#f8fafc') || '#f8fafc';
var currentThemeDepth = parseFloat(safeGetStorage('outfit_wheeler_theme_depth', '0.60')) || 0.60;
var currentPresetId = safeGetStorage('outfit_wheeler_theme_preset', 'sapphire') || 'sapphire';

function hexToRgb(hex) {
    var c = (hex || '#38bdf8').replace('#', '');
    if (c.length === 3) c = c.split('').map(function(x) { return x + x; }).join('');
    var num = parseInt(c, 16) || 0;
    return { r: (num >> 16) & 255, g: (num >> 8) & 255, b: num & 255 };
}

function rgbToHsl(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    var max = Math.max(r, g, b), min = Math.min(r, g, b);
    var h, s, l = (max + min) / 2;
    if (max === min) {
        h = s = 0;
    } else {
        var d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        switch (max) {
            case r: h = (g - b) / d + (g < b ? 6 : 0); break;
            case g: h = (b - r) / d + 2; break;
            case b: h = (r - g) / d + 4; break;
        }
        h = Math.round(h * 60);
    }
    return { h: h, s: Math.round(s * 100), l: Math.round(l * 100) };
}

function hslToRgb(h, s, l) {
    h = (((h % 360) + 360) % 360) / 360;
    s = Math.max(0, Math.min(100, s)) / 100;
    l = Math.max(0, Math.min(100, l)) / 100;

    var r, g, b;
    if (s === 0) {
        r = g = b = l;
    } else {
        var hue2rgb = function(p, q, t) {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1/6) return p + (q - p) * 6 * t;
            if (t < 1/2) return q;
            if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
        };
        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;
        r = hue2rgb(p, q, h + 1/3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1/3);
    }
    return { r: Math.round(r * 255), g: Math.round(g * 255), b: Math.round(b * 255) };
}

function applyTheme(hex, depth, presetId, bgHex, textHex) {
    if (hex) currentThemeHex = hex.toLowerCase();
    if (bgHex) currentBgHex = bgHex.toLowerCase();
    if (textHex) currentTextHex = textHex.toLowerCase();
    if (depth !== undefined && depth !== null) currentThemeDepth = Math.max(0, Math.min(1, depth));
    if (presetId !== undefined && presetId !== null) {
        currentPresetId = presetId;
    } else {
        var matched = GEM_PRESETS.find(function(p) {
            return p.hex.toLowerCase() === currentThemeHex && (!p.bgHex || p.bgHex.toLowerCase() === currentBgHex);
        });
        currentPresetId = matched ? matched.id : 'custom';
    }

    safeSetStorage('outfit_wheeler_theme_hex', currentThemeHex);
    safeSetStorage('outfit_wheeler_theme_bg', currentBgHex);
    safeSetStorage('outfit_wheeler_theme_text', currentTextHex);
    safeSetStorage('outfit_wheeler_theme_depth', String(currentThemeDepth));
    safeSetStorage('outfit_wheeler_theme_preset', currentPresetId);

    var rgbA = hexToRgb(currentThemeHex);
    var hslA = rgbToHsl(rgbA.r, rgbA.g, rgbA.b);
    var d = currentThemeDepth;
    var hA = hslA.h, sA = hslA.s;

    var rgbB = hexToRgb(currentBgHex);
    var hslB = rgbToHsl(rgbB.r, rgbB.g, rgbB.b);
    var hB = hslB.h, sB = hslB.s, lB = hslB.l;

    var rgbT = hexToRgb(currentTextHex);
    var hslT = rgbToHsl(rgbT.r, rgbT.g, rgbT.b);
    var hT = hslT.h, sT = hslT.s, lT = hslT.l;

    if (typeof document !== 'undefined' && document.documentElement) {
        var root = document.documentElement;
        // 1. Accent & Glow
        root.style.setProperty('--accent-hex', currentThemeHex);
        root.style.setProperty('--accent-primary', currentThemeHex);
        root.style.setProperty('--border-active', currentThemeHex);
        root.style.setProperty('--color-equipped', currentThemeHex);
        root.style.setProperty('--accent-glow', 'hsla(' + hA + ', 95%, 60%, ' + (0.25 + d * 0.35) + ')');
        root.style.setProperty('--accent-glow-disc', 'hsla(' + hA + ', 90%, 55%, ' + (0.18 + d * 0.25) + ')');
        root.style.setProperty('--accent-subtle', 'hsla(' + hA + ', ' + Math.max(sA, 50) + '%, 50%, 0.16)');
        root.style.setProperty('--border-hover', 'hsla(' + hA + ', ' + (sA * 0.8) + '%, ' + (55 + d * 10) + '%, 0.70)');
        root.style.setProperty('--slice-stroke-hover', currentThemeHex);
        root.style.setProperty('--slice-fill-hover', 'hsla(' + hA + ', 75%, 45%, 0.25)');
        root.style.setProperty('--slice-stroke-equipped', currentThemeHex);
        root.style.setProperty('--slice-fill-equipped', 'hsla(' + hA + ', 80%, 40%, 0.35)');

        // 2. Menu Backgrounds & Surfaces
        if (lB <= 50) {
            root.style.setProperty('--bg-primary', 'hsla(' + hB + ', ' + sB + '%, ' + Math.max(1, lB - 3) + '%, 0.98)');
            root.style.setProperty('--bg-surface', 'hsla(' + hB + ', ' + sB + '%, ' + lB + '%, 0.94)');
            root.style.setProperty('--bg-card', 'hsla(' + hB + ', ' + sB + '%, ' + Math.min(95, lB + 5) + '%, 0.90)');
            root.style.setProperty('--bg-card-hover', 'hsla(' + hB + ', ' + sB + '%, ' + Math.min(95, lB + 11) + '%, 0.96)');
            root.style.setProperty('--border-color', 'hsla(' + hB + ', ' + Math.max(15, sB * 0.6) + '%, ' + Math.min(85, lB + 18) + '%, ' + (0.35 + d * 0.20) + ')');
            root.style.setProperty('--slice-fill-default', 'hsla(' + hB + ', ' + sB + '%, ' + lB + '%, 0.88)');
            root.style.setProperty('--slice-stroke-default', 'hsla(' + hB + ', ' + Math.max(15, sB * 0.5) + '%, ' + Math.min(85, lB + 22) + '%, 0.50)');
        } else {
            root.style.setProperty('--bg-primary', 'hsla(' + hB + ', ' + sB + '%, ' + Math.max(5, lB - 10) + '%, 0.98)');
            root.style.setProperty('--bg-surface', 'hsla(' + hB + ', ' + sB + '%, ' + lB + '%, 0.94)');
            root.style.setProperty('--bg-card', 'hsla(' + hB + ', ' + sB + '%, ' + Math.max(5, lB - 6) + '%, 0.92)');
            root.style.setProperty('--bg-card-hover', 'hsla(' + hB + ', ' + sB + '%, ' + Math.max(5, lB - 12) + '%, 0.96)');
            root.style.setProperty('--border-color', 'hsla(' + hB + ', ' + Math.max(15, sB * 0.6) + '%, ' + Math.max(5, lB - 25) + '%, ' + (0.40 + d * 0.20) + ')');
            root.style.setProperty('--slice-fill-default', 'hsla(' + hB + ', ' + sB + '%, ' + lB + '%, 0.88)');
            root.style.setProperty('--slice-stroke-default', 'hsla(' + hB + ', ' + Math.max(15, sB * 0.5) + '%, ' + Math.max(5, lB - 25) + '%, 0.55)');
        }

        // 3. Text & Typography
        root.style.setProperty('--text-primary', currentTextHex);
        if (lT >= 50) {
            root.style.setProperty('--text-secondary', 'hsla(' + hT + ', ' + Math.max(5, sT * 0.35) + '%, ' + Math.max(20, lT - 22) + '%, 0.85)');
            root.style.setProperty('--text-muted', 'hsla(' + hT + ', ' + Math.max(5, sT * 0.30) + '%, ' + Math.max(15, lT - 38) + '%, 0.70)');
        } else {
            root.style.setProperty('--text-secondary', 'hsla(' + hT + ', ' + Math.max(5, sT * 0.35) + '%, ' + Math.min(85, lT + 22) + '%, 0.85)');
            root.style.setProperty('--text-muted', 'hsla(' + hT + ', ' + Math.max(5, sT * 0.30) + '%, ' + Math.min(90, lT + 38) + '%, 0.70)');
        }
        root.style.setProperty('--text-gold', 'hsl(' + hA + ', 85%, 65%)');

        // UI Indicators in Settings panel
        var previewChip = document.getElementById('theme-color-preview-chip');
        if (previewChip) {
            previewChip.style.background = currentThemeHex;
            previewChip.style.boxShadow = '0 0 10px ' + currentThemeHex;
        }
        var hexText = document.getElementById('theme-color-hex');
        if (hexText) hexText.innerText = currentThemeHex.toUpperCase();

        var bgChip = document.getElementById('theme-bg-preview-chip');
        if (bgChip) {
            bgChip.style.background = currentBgHex;
            bgChip.style.boxShadow = '0 0 10px rgba(0,0,0,0.5)';
        }
        var bgHexText = document.getElementById('theme-bg-hex');
        if (bgHexText) bgHexText.innerText = currentBgHex.toUpperCase();

        var textChip = document.getElementById('theme-text-preview-chip');
        if (textChip) {
            textChip.style.background = currentTextHex;
            textChip.style.boxShadow = '0 0 10px ' + currentTextHex;
        }
        var textHexText = document.getElementById('theme-text-hex');
        if (textHexText) textHexText.innerText = currentTextHex.toUpperCase();

        var depthSlider = document.getElementById('slider-theme-depth');
        if (depthSlider) depthSlider.value = Math.round(d * 100);
        var depthTxt = document.getElementById('txt-theme-depth');
        if (depthTxt) depthTxt.innerText = Math.round(d * 100) + '%';

        var chips = document.querySelectorAll('#theme-preset-pills .theme-preset-chip');
        chips.forEach(function(c) {
            if (c.getAttribute('data-id') === currentPresetId) c.classList.add('active');
            else c.classList.remove('active');
        });
    }

    if (typeof updateWheelUI === 'function') updateWheelUI();
}

function renderThemePresets() {
    var container = document.getElementById('theme-preset-pills');
    if (!container) return;
    container.innerHTML = '';
    GEM_PRESETS.forEach(function(p) {
        var chip = document.createElement('div');
        chip.className = 'theme-preset-chip' + (p.id === currentPresetId ? ' active' : '');
        chip.setAttribute('data-id', p.id);
        chip.onclick = function() { selectThemePreset(p.id); };
        chip.innerHTML = '<span class="theme-preset-dot" style="background:' + p.hex + ';"></span><span>' + p.name.split(' ')[0] + '</span>';
        container.appendChild(chip);
    });
}

function selectThemePreset(presetId) {
    var p = GEM_PRESETS.find(function(item) { return item.id === presetId; });
    if (p) {
        applyTheme(p.hex, p.depth, p.id, p.bgHex, p.textHex);
    }
}

function onThemeColorPick(hex) {
    applyTheme(hex, currentThemeDepth, null, currentBgHex, currentTextHex);
}

function onThemeDepthInput(depthVal) {
    var d = parseFloat(depthVal) / 100.0;
    applyTheme(currentThemeHex, d, currentPresetId, currentBgHex, currentTextHex);
}

/* ==========================================================================
   2D COLOR SPECTRUM CANVAS & MODAL (PER-SLOT, PER-WHEEL & GLOBAL THEME)
   ========================================================================== */
var colorPickerTarget = null;
var selectedModalColor = '#38bdf8';
var isSpectrumPicking = false;

function initColorSpectrumCanvas() {
    if (typeof document === 'undefined') return;
    var canvas = document.getElementById('color-spectrum-canvas');
    if (!canvas || typeof canvas.getContext !== 'function') return;
    var ctx = canvas.getContext('2d');
    if (!ctx) return;
    var w = canvas.width;
    var h = canvas.height;

    // Horizontal rainbow spectrum
    var hGrad = ctx.createLinearGradient(0, 0, w, 0);
    hGrad.addColorStop(0.00, '#ff0000');
    hGrad.addColorStop(0.17, '#ff8800');
    hGrad.addColorStop(0.33, '#ffff00');
    hGrad.addColorStop(0.50, '#00ff00');
    hGrad.addColorStop(0.66, '#00ffff');
    hGrad.addColorStop(0.83, '#0000ff');
    hGrad.addColorStop(0.92, '#ff00ff');
    hGrad.addColorStop(1.00, '#ff0000');
    ctx.fillStyle = hGrad;
    ctx.fillRect(0, 0, w, h);

    // Vertical black overlay (transparent at top -> black at bottom)
    var vGrad = ctx.createLinearGradient(0, 0, 0, h);
    vGrad.addColorStop(0.0, 'rgba(0, 0, 0, 0)');
    vGrad.addColorStop(0.15, 'rgba(0, 0, 0, 0)');
    vGrad.addColorStop(1.0, '#000000');
    ctx.fillStyle = vGrad;
    ctx.fillRect(0, 0, w, h);
}

function setupSpectrumEvents() {
    if (typeof document === 'undefined') return;
    var wrapper = document.getElementById('color-spectrum-wrapper');
    if (!wrapper || wrapper._hasEvents) return;
    wrapper._hasEvents = true;

    function handlePick(e) {
        var rect = wrapper.getBoundingClientRect();
        var clientX = e.clientX !== undefined ? e.clientX : (e.touches && e.touches[0] ? e.touches[0].clientX : 0);
        var clientY = e.clientY !== undefined ? e.clientY : (e.touches && e.touches[0] ? e.touches[0].clientY : 0);
        
        var x = Math.max(0, Math.min(rect.width, clientX - rect.left));
        var y = Math.max(0, Math.min(rect.height, clientY - rect.top));
        
        pickColorAtCoords(x, y, rect.width, rect.height);
    }

    wrapper.addEventListener('mousedown', function(e) {
        if (e.button !== 0) return;
        isSpectrumPicking = true;
        handlePick(e);
        e.preventDefault();
    });

    window.addEventListener('mousemove', function(e) {
        if (!isSpectrumPicking) return;
        handlePick(e);
    });

    window.addEventListener('mouseup', function() {
        isSpectrumPicking = false;
    });

    wrapper.addEventListener('touchstart', function(e) {
        isSpectrumPicking = true;
        handlePick(e);
    }, { passive: true });

    window.addEventListener('touchmove', function(e) {
        if (!isSpectrumPicking) return;
        handlePick(e);
    }, { passive: true });

    window.addEventListener('touchend', function() {
        isSpectrumPicking = false;
    });
}

function pickColorAtCoords(x, y, w, h) {
    if (typeof document === 'undefined') return;
    var canvas = document.getElementById('color-spectrum-canvas');
    if (!canvas) return;
    var ctx = (typeof canvas.getContext === 'function') ? canvas.getContext('2d') : null;
    if (!ctx) {
        var hue = (x / w) * 360;
        var val = 1 - (y / h);
        var rgb = hslToRgb(hue, 100, Math.round(val * 50));
        selectedModalColor = '#' + ((1 << 24) + (rgb.r << 16) + (rgb.g << 8) + rgb.b).toString(16).slice(1).toLowerCase();
        updateModalColorUI(x, y);
        return;
    }

    var canvasX = Math.floor((x / w) * canvas.width);
    var canvasY = Math.floor((y / h) * canvas.height);
    canvasX = Math.max(0, Math.min(canvas.width - 1, canvasX));
    canvasY = Math.max(0, Math.min(canvas.height - 1, canvasY));

    try {
        var pixel = ctx.getImageData(canvasX, canvasY, 1, 1).data;
        var r = pixel[0], g = pixel[1], b = pixel[2];
        var hex = '#' + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
        selectedModalColor = hex.toLowerCase();
    } catch (err) {
        var hue = (x / w) * 360;
        var val = 1 - (y / h);
        var rgb = hslToRgb(hue, 100, val * 50);
        selectedModalColor = '#' + ((1 << 24) + (rgb.r << 16) + (rgb.g << 8) + rgb.b).toString(16).slice(1).toLowerCase();
    }

    if (colorPickerTarget && colorPickerTarget.type === 'theme') {
        if (colorPickerTarget.mode === 'bg') {
            applyTheme(currentThemeHex, currentThemeDepth, null, selectedModalColor, currentTextHex);
        } else if (colorPickerTarget.mode === 'text') {
            applyTheme(currentThemeHex, currentThemeDepth, null, currentBgHex, selectedModalColor);
        } else {
            applyTheme(selectedModalColor, currentThemeDepth, null, currentBgHex, currentTextHex);
        }
    }

    updateModalColorUI(x, y);
}

function syncHandleToColor(hex) {
    if (typeof document === 'undefined') return;
    var wrapper = document.getElementById('color-spectrum-wrapper');
    var handle = document.getElementById('color-spectrum-handle');
    if (!wrapper || !handle) return;

    var rgb = hexToRgb(hex);
    var hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    var w = wrapper.clientWidth || 360;
    var h = wrapper.clientHeight || 180;

    var x = Math.max(0, Math.min(w, (hsl.h / 360) * w));
    var y = Math.max(0, Math.min(h, (1 - (hsl.l / 100)) * h));

    handle.style.left = Math.round(x) + 'px';
    handle.style.top = Math.round(y) + 'px';
}

function updateModalColorUI(handleX, handleY) {
    if (typeof document === 'undefined') return;
    var handle = document.getElementById('color-spectrum-handle');
    var wrapper = document.getElementById('color-spectrum-wrapper');
    if (handle && wrapper) {
        if (handleX !== undefined && handleY !== undefined) {
            handle.style.left = handleX + 'px';
            handle.style.top = handleY + 'px';
        } else {
            syncHandleToColor(selectedModalColor);
        }
    }

    var preview = document.getElementById('modal-color-preview');
    if (preview) {
        preview.style.background = selectedModalColor;
        preview.style.boxShadow = '0 0 16px ' + selectedModalColor;
    }

    var hexInput = document.getElementById('modal-color-hex-input');
    if (hexInput && document.activeElement !== hexInput) {
        hexInput.value = selectedModalColor.replace('#', '').toUpperCase();
    }

    var hexLbl = document.getElementById('modal-color-hex-label');
    if (hexLbl) {
        hexLbl.innerText = selectedModalColor.toUpperCase();
        hexLbl.style.color = selectedModalColor;
    }

    var nameBadge = document.getElementById('modal-color-name-label');
    if (nameBadge) {
        var matched = GEM_PRESETS.find(function(p) { return p.hex.toLowerCase() === selectedModalColor.toLowerCase(); });
        nameBadge.innerText = matched ? matched.name : 'Custom Hex';
    }

    var swatches = document.querySelectorAll('#modal-color-swatches .color-swatch-item');
    swatches.forEach(function(s) {
        var sHex = s.getAttribute('data-hex');
        if (sHex && sHex.toLowerCase() === selectedModalColor.toLowerCase()) {
            s.classList.add('active');
        } else {
            s.classList.remove('active');
        }
    });
}

function onModalHexTextInput(val) {
    if (!val) return;
    val = val.trim().replace(/[^0-9a-fA-F]/g, '');
    if (val.length === 3) {
        val = val[0] + val[0] + val[1] + val[1] + val[2] + val[2];
    }
    if (val.length === 6) {
        selectedModalColor = ('#' + val).toLowerCase();
        if (colorPickerTarget && colorPickerTarget.type === 'theme') {
            if (colorPickerTarget.mode === 'bg') {
                applyTheme(currentThemeHex, currentThemeDepth, null, selectedModalColor, currentTextHex);
            } else if (colorPickerTarget.mode === 'text') {
                applyTheme(currentThemeHex, currentThemeDepth, null, currentBgHex, selectedModalColor);
            } else {
                applyTheme(selectedModalColor, currentThemeDepth, null, currentBgHex, currentTextHex);
            }
        }
        updateModalColorUI();
    }
}

function openSlotColorModal(e) {
    if (e) {
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
        if (typeof e.preventDefault === 'function') e.preventDefault();
    }
    hideAllMenus();
    var modal = document.getElementById('color-picker-modal');
    if (!modal || !currentWheelData) return;

    var tabs = document.getElementById('color-target-tabs');
    if (tabs) tabs.style.display = 'none';

    var slot = (currentWheelData.slots || []).find(function(s) { return (s.index !== undefined ? s.index : 0) === selectedSlotIndex; }) || (currentWheelData.slots || [])[selectedSlotIndex];
    selectedModalColor = (slot && slot.color && slot.color.trim().length > 0) ? slot.color : (currentWheelData.wheelColor || currentThemeHex || '#38bdf8');
    colorPickerTarget = { type: 'slot', wheelIndex: (currentWheelData.wheelIndex || 0), slotIndex: selectedSlotIndex };

    var titleEl = document.getElementById('color-modal-title');
    if (titleEl) titleEl.innerText = 'Customize Slot ' + ((selectedSlotIndex !== null ? selectedSlotIndex + 1 : 1)) + ' Color';
    var descEl = document.getElementById('color-modal-desc');
    if (descEl) descEl.innerText = 'Pick any custom color on the spectrum, or enter hex. Overrides the wheel & global theme for this slot.';

    modal.style.display = 'flex';
    setTyping(true);
    initColorSpectrumCanvas();
    setupSpectrumEvents();
    renderModalColorSwatches();
    updateModalColorUI();
}

function openWheelColorModal(e) {
    if (e) {
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
        if (typeof e.preventDefault === 'function') e.preventDefault();
    }
    hideAllMenus();
    var modal = document.getElementById('color-picker-modal');
    if (!modal || !currentWheelData) return;

    var tabs = document.getElementById('color-target-tabs');
    if (tabs) tabs.style.display = 'none';

    selectedModalColor = (currentWheelData.wheelColor && currentWheelData.wheelColor.trim().length > 0) ? currentWheelData.wheelColor : (currentThemeHex || '#38bdf8');
    colorPickerTarget = { type: 'wheel', wheelIndex: (currentWheelData.wheelIndex || 0) };

    var titleEl = document.getElementById('color-modal-title');
    if (titleEl) titleEl.innerText = 'Customize Wheel ' + (((currentWheelData.wheelIndex || 0) + 1)) + ' Color';
    var descEl = document.getElementById('color-modal-desc');
    if (descEl) descEl.innerText = 'Pick any custom color on the spectrum. All slots in this wheel inherit this archetype unless individually overridden.';

    modal.style.display = 'flex';
    setTyping(true);
    initColorSpectrumCanvas();
    setupSpectrumEvents();
    renderModalColorSwatches();
    updateModalColorUI();
}

function updateColorTargetTabUI(mode) {
    ['accent', 'bg', 'text'].forEach(function(m) {
        var btn = document.getElementById('tab-color-' + m);
        if (btn) {
            if (m === mode) btn.classList.add('active');
            else btn.classList.remove('active');
        }
    });
}

function switchThemeColorMode(mode) {
    if (!colorPickerTarget || colorPickerTarget.type !== 'theme') return;
    colorPickerTarget.mode = mode;
    updateColorTargetTabUI(mode);
    if (mode === 'bg') selectedModalColor = currentBgHex || '#070a0f';
    else if (mode === 'text') selectedModalColor = currentTextHex || '#f8fafc';
    else selectedModalColor = currentThemeHex || '#38bdf8';

    var titleEl = document.getElementById('color-modal-title');
    if (titleEl) {
        if (mode === 'bg') titleEl.innerText = 'Customize Menu Background';
        else if (mode === 'text') titleEl.innerText = 'Customize Text Color';
        else titleEl.innerText = 'Customize Accent & Glow';
    }
    var descEl = document.getElementById('color-modal-desc');
    if (descEl) {
        if (mode === 'bg') descEl.innerText = 'Pick any custom color for menu panels, drawers, popups and cards from the spectrum.';
        else if (mode === 'text') descEl.innerText = 'Pick any custom color for titles, labels, item names and text typography.';
        else descEl.innerText = 'Pick any custom color for glows, active borders, buttons, and wheel accents.';
    }

    syncHandleToColor(selectedModalColor);
    renderModalColorSwatches();
    updateModalColorUI();
}

function openThemeColorModal(targetMode, e) {
    if (typeof targetMode === 'object' && targetMode !== null) {
        e = targetMode;
        targetMode = 'accent';
    }
    if (!targetMode || typeof targetMode !== 'string') targetMode = 'accent';
    if (e) {
        if (typeof e.stopPropagation === 'function') e.stopPropagation();
        if (typeof e.preventDefault === 'function') e.preventDefault();
    }
    hideAllMenus();
    var modal = document.getElementById('color-picker-modal');
    if (!modal) return;

    colorPickerTarget = { type: 'theme', mode: targetMode };
    if (targetMode === 'bg') selectedModalColor = currentBgHex || '#070a0f';
    else if (targetMode === 'text') selectedModalColor = currentTextHex || '#f8fafc';
    else selectedModalColor = currentThemeHex || '#38bdf8';

    var tabs = document.getElementById('color-target-tabs');
    if (tabs) {
        tabs.style.display = 'flex';
        updateColorTargetTabUI(targetMode);
    }

    var titleEl = document.getElementById('color-modal-title');
    if (titleEl) {
        if (targetMode === 'bg') titleEl.innerText = 'Customize Menu Background';
        else if (targetMode === 'text') titleEl.innerText = 'Customize Text Color';
        else titleEl.innerText = 'Customize Accent & Glow';
    }
    var descEl = document.getElementById('color-modal-desc');
    if (descEl) {
        if (targetMode === 'bg') descEl.innerText = 'Pick any custom color for menu panels, drawers, popups and cards from the spectrum.';
        else if (targetMode === 'text') descEl.innerText = 'Pick any custom color for titles, labels, item names and text typography.';
        else descEl.innerText = 'Pick any custom color for glows, active borders, buttons, and wheel accents.';
    }

    modal.style.display = 'flex';
    setTyping(true);
    initColorSpectrumCanvas();
    setupSpectrumEvents();
    renderModalColorSwatches();
    updateModalColorUI();
}

function hideColorPickerModal() {
    var modal = document.getElementById('color-picker-modal');
    if (modal) modal.style.display = 'none';
    colorPickerTarget = null;
    setTyping(false);
}

var THEME_BG_SWATCHES = [
    { name: 'Obsidian', hex: '#070a0f' },
    { name: 'Navy', hex: '#070b14' },
    { name: 'Crimson', hex: '#180608' },
    { name: 'Void', hex: '#12071f' },
    { name: 'Forest', hex: '#05190e' },
    { name: 'Timber', hex: '#180e05' },
    { name: 'Slate', hex: '#061318' },
    { name: 'Blood', hex: '#18060d' },
    { name: 'OLED', hex: '#000000' },
    { name: 'Parchment', hex: '#1f1912' },
    { name: 'Graphite', hex: '#111827' }
];

var THEME_TEXT_SWATCHES = [
    { name: 'Crisp Ice', hex: '#f8fafc' },
    { name: 'Frost Sky', hex: '#f0f9ff' },
    { name: 'Daedric Bone', hex: '#fff1f2' },
    { name: 'Mystic Lav', hex: '#faf5ff' },
    { name: 'Mint White', hex: '#ecfdf5' },
    { name: 'Warm Cream', hex: '#fefce8' },
    { name: 'Cyan White', hex: '#ecfeff' },
    { name: 'Platinum', hex: '#ffffff' },
    { name: 'Sepia Gold', hex: '#fef3c7' },
    { name: 'Silver Mist', hex: '#e2e8f0' }
];

function renderModalColorSwatches() {
    var container = document.getElementById('modal-color-swatches');
    if (!container) return;
    container.innerHTML = '';
    var list = GEM_PRESETS;
    if (colorPickerTarget && colorPickerTarget.type === 'theme') {
        if (colorPickerTarget.mode === 'bg') list = THEME_BG_SWATCHES;
        else if (colorPickerTarget.mode === 'text') list = THEME_TEXT_SWATCHES;
    }
    list.forEach(function(p) {
        var item = document.createElement('div');
        item.className = 'color-swatch-item' + (p.hex.toLowerCase() === selectedModalColor.toLowerCase() ? ' active' : '');
        item.setAttribute('data-hex', p.hex);
        item.onclick = function() { selectModalPresetColor(p.hex); };
        item.innerHTML = '<span class="theme-preset-dot" style="background:' + p.hex + ';"></span><span>' + p.name.split(' ')[0] + '</span>';
        container.appendChild(item);
    });
}

function selectModalPresetColor(hex) {
    selectedModalColor = hex.toLowerCase();
    if (colorPickerTarget && colorPickerTarget.type === 'theme') {
        if (colorPickerTarget.mode === 'bg') {
            applyTheme(currentThemeHex, currentThemeDepth, null, selectedModalColor, currentTextHex);
        } else if (colorPickerTarget.mode === 'text') {
            applyTheme(currentThemeHex, currentThemeDepth, null, currentBgHex, selectedModalColor);
        } else {
            applyTheme(selectedModalColor, currentThemeDepth, null, currentBgHex, currentTextHex);
        }
    }
    updateModalColorUI();
}

function onModalColorInput(val) {
    selectedModalColor = val.toLowerCase();
    if (colorPickerTarget && colorPickerTarget.type === 'theme') {
        if (colorPickerTarget.mode === 'bg') {
            applyTheme(currentThemeHex, currentThemeDepth, null, selectedModalColor, currentTextHex);
        } else if (colorPickerTarget.mode === 'text') {
            applyTheme(currentThemeHex, currentThemeDepth, null, currentBgHex, selectedModalColor);
        } else {
            applyTheme(selectedModalColor, currentThemeDepth, null, currentBgHex, currentTextHex);
        }
    }
    updateModalColorUI();
}

function resetTargetColor() {
    if (!colorPickerTarget) return;
    if (colorPickerTarget.type === 'slot') {
        if (!currentWheelData) return;
        var sIdx = colorPickerTarget.slotIndex;
        var slot = (currentWheelData.slots || []).find(function(s) { return (s.index !== undefined ? s.index : 0) === sIdx; }) || (currentWheelData.slots || [])[sIdx];
        if (slot) slot.color = '';
        if (typeof window.SetSlotColor === 'function') {
            window.SetSlotColor(JSON.stringify({ wheelIndex: colorPickerTarget.wheelIndex, slotIndex: sIdx, color: '' }));
        }
        showSaveToast('Slot Color Reset to Default');
    } else if (colorPickerTarget.type === 'wheel') {
        if (!currentWheelData) return;
        currentWheelData.wheelColor = '';
        if (typeof window.SetWheelColor === 'function') {
            window.SetWheelColor(JSON.stringify({ wheelIndex: colorPickerTarget.wheelIndex, color: '' }));
        }
        showSaveToast('Wheel Color Reset to Default');
    } else if (colorPickerTarget.type === 'theme') {
        if (colorPickerTarget.mode === 'bg') {
            applyTheme(currentThemeHex, currentThemeDepth, null, '#070a0f', currentTextHex);
            showSaveToast('Menu Background Reset to Obsidian Dark');
        } else if (colorPickerTarget.mode === 'text') {
            applyTheme(currentThemeHex, currentThemeDepth, null, currentBgHex, '#f8fafc');
            showSaveToast('Text Color Reset to Crisp White');
        } else {
            applyTheme('#38bdf8', 0.60, 'sapphire', currentBgHex, currentTextHex);
            showSaveToast('Accent Color Reset to Sapphire Blue');
        }
    }
    hideColorPickerModal();
    if (typeof updateWheelUI === 'function') updateWheelUI();
    if (typeof renderDrawerDetails === 'function') renderDrawerDetails();
    if (typeof renderSlotsRail === 'function') renderSlotsRail();
}

function confirmTargetColor() {
    if (!colorPickerTarget) return;
    var chosen = selectedModalColor.toLowerCase();
    if (colorPickerTarget.type === 'slot') {
        if (!currentWheelData) return;
        var sIdx = colorPickerTarget.slotIndex;
        var slot = (currentWheelData.slots || []).find(function(s) { return (s.index !== undefined ? s.index : 0) === sIdx; }) || (currentWheelData.slots || [])[sIdx];
        if (slot) slot.color = chosen;
        if (typeof window.SetSlotColor === 'function') {
            window.SetSlotColor(JSON.stringify({ wheelIndex: colorPickerTarget.wheelIndex, slotIndex: sIdx, color: chosen }));
        }
        showSaveToast('Slot Color Saved!');
    } else if (colorPickerTarget.type === 'wheel') {
        if (!currentWheelData) return;
        currentWheelData.wheelColor = chosen;
        if (typeof window.SetWheelColor === 'function') {
            window.SetWheelColor(JSON.stringify({ wheelIndex: colorPickerTarget.wheelIndex, color: chosen }));
        }
        showSaveToast('Wheel Color Saved!');
    } else if (colorPickerTarget.type === 'theme') {
        if (colorPickerTarget.mode === 'bg') {
            applyTheme(currentThemeHex, currentThemeDepth, null, chosen, currentTextHex);
            showSaveToast('Menu Background Color Saved!');
        } else if (colorPickerTarget.mode === 'text') {
            applyTheme(currentThemeHex, currentThemeDepth, null, currentBgHex, chosen);
            showSaveToast('Text Color Saved!');
        } else {
            applyTheme(chosen, currentThemeDepth, null, currentBgHex, currentTextHex);
            showSaveToast('Accent Color Saved!');
        }
    }
    hideColorPickerModal();
    if (typeof updateWheelUI === 'function') updateWheelUI();
    if (typeof renderDrawerDetails === 'function') renderDrawerDetails();
    if (typeof renderSlotsRail === 'function') renderSlotsRail();
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

    renderThemePresets();
    const chkPreviews = document.getElementById('chk-enable-previews');
    if (chkPreviews) chkPreviews.checked = (typeof isItemPreviewsEnabled === 'function') ? isItemPreviewsEnabled() : true;
    applyTheme(currentThemeHex, currentThemeDepth, currentPresetId, currentBgHex, currentTextHex);

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

    const chkPreviews = document.getElementById('chk-enable-previews');
    const previewsVal = chkPreviews ? chkPreviews.checked : true;
    currentWheelData.showItemPreviews = previewsVal;
    safeSetStorage('outfit_wheeler_show_previews', String(previewsVal));

    safeSetStorage('outfit_wheeler_theme_hex', currentThemeHex);
    safeSetStorage('outfit_wheeler_theme_depth', String(currentThemeDepth));
    safeSetStorage('outfit_wheeler_theme_preset', currentPresetId);
    applyTheme(currentThemeHex, currentThemeDepth, currentPresetId);

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
            autoSwimEnabled: currentWheelData.autoSwimEnabled !== false,
            showItemPreviews: previewsVal,
            themeAccentColor: currentThemeHex,
            themeBgColor: currentBgHex,
            themeTextColor: currentTextHex,
            themeDepth: currentThemeDepth,
            themePreset: currentPresetId
        }));
    }
    hideSettingsModal();
    if (typeof updateUiForPreviewsSetting === 'function') updateUiForPreviewsSetting();
    if (typeof renderInventoryList === 'function') renderInventoryList();
    if (typeof updateWheelUI === 'function') updateWheelUI();
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
    if (typeof window.ExportConfig === 'function') {
        window.ExportConfig('{}');
    } else if (window.Prisma && typeof window.Prisma.call === 'function') {
        window.Prisma.call('ExportConfig', '{}');
    } else {
        console.log('[OutfitWheeler] ExportConfig requested in test/mock mode');
    }
}

function importConfiguration(backupPath) {
    const payload = JSON.stringify(backupPath ? { path: backupPath } : { auto: true });
    if (typeof window.ImportConfig === 'function') {
        window.ImportConfig(payload);
    } else if (window.Prisma && typeof window.Prisma.call === 'function') {
        window.Prisma.call('ImportConfig', payload);
    } else {
        console.log('[OutfitWheeler] ImportConfig requested in test/mock mode');
    }
}

function clearThumbnailCache() {
    if (typeof window.ClearThumbnailCache === 'function') {
        window.ClearThumbnailCache('{}');
    } else if (window.Prisma && typeof window.Prisma.call === 'function') {
        window.Prisma.call('ClearThumbnailCache', '{}');
    } else {
        console.log('[OutfitWheeler] ClearThumbnailCache requested in test/mock mode');
    }
}

function fetchBackupsList() {
    if (typeof window.GetBackupsList === 'function') {
        window.GetBackupsList('{}');
    } else if (window.Prisma && typeof window.Prisma.call === 'function') {
        window.Prisma.call('GetBackupsList', '{}');
    } else {
        console.log('[OutfitWheeler] GetBackupsList requested in test/mock mode');
    }
}

function openBackupsModal() {
    const modal = document.getElementById('backups-modal');
    if (modal) {
        modal.style.display = 'flex';
        const listEl = document.getElementById('backups-list-container');
        if (listEl) {
            listEl.innerHTML = '<div style="padding: 16px; text-align: center; color: var(--text-muted); font-size: 12px;">Loading backups...</div>';
        }
        fetchBackupsList();
    }
}

function hideBackupsModal() {
    const modal = document.getElementById('backups-modal');
    if (modal) {
        modal.style.display = 'none';
    }
}

function restoreSpecificBackup(path) {
    if (!path) return;
    importConfiguration(path);
    hideBackupsModal();
}

function onBackupsListReceived(data) {
    try {
        const list = typeof data === 'string' ? JSON.parse(data) : data;
        const listEl = document.getElementById('backups-list-container');
        if (!listEl) return;
        if (!Array.isArray(list) || list.length === 0) {
            listEl.innerHTML = '<div style="padding: 24px; text-align: center; color: var(--text-muted); font-size: 12px;">No backups found in Data/SKSE/Plugins/backups/</div>';
            return;
        }
        let html = '';
        list.forEach(b => {
            const sizeKb = b.size ? (b.size / 1024).toFixed(1) + ' KB' : '';
            const safePath = (b.path || '').replace(/\\/g, '/').replace(/'/g, "\\'");
            html += `
            <div style="display: flex; align-items: center; justify-content: space-between; padding: 10px 14px; background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.06); border-radius: 6px; margin-bottom: 8px;">
                <div style="display: flex; flex-direction: column; gap: 2px; overflow: hidden; margin-right: 12px;">
                    <span style="font-weight: 600; font-size: 12px; color: #f1f5f9; text-overflow: ellipsis; white-space: nowrap; overflow: hidden;">${b.filename}</span>
                    <span style="font-size: 10px; color: #94a3b8;">${sizeKb}</span>
                </div>
                <button type="button" class="btn-primary" onclick="restoreSpecificBackup('${safePath}')" style="padding: 4px 12px; font-size: 11px; white-space: nowrap;">
                    Restore
                </button>
            </div>`;
        });
        listEl.innerHTML = html;
    } catch (err) {
        console.error('Error rendering backups list:', err);
    }
}

function onConfigExported(data) {
    showSaveToast("Backup exported successfully!");
}

function onConfigImported(data) {
    try {
        const res = typeof data === 'string' ? JSON.parse(data) : data;
        if (res && res.success) {
            showSaveToast("Configuration restored successfully!");
        } else {
            showSaveToast("Failed to restore configuration.");
        }
    } catch (_) {
        showSaveToast("Configuration restored!");
    }
}

function onThumbnailCacheCleared(data) {
    try {
        const res = typeof data === 'string' ? JSON.parse(data) : data;
        const count = res && res.removed !== undefined ? res.removed : 0;
        showSaveToast(`Thumbnail cache cleared (${count} files)!`);
    } catch (_) {
        showSaveToast("Thumbnail cache cleared!");
    }
    if (typeof window.clearLoadedThumbnails === 'function') {
        window.clearLoadedThumbnails();
    }
}

if (typeof window !== 'undefined') {
    window.exportConfiguration = exportConfiguration;
    window.importConfiguration = importConfiguration;
    window.clearThumbnailCache = clearThumbnailCache;
    window.fetchBackupsList = fetchBackupsList;
    window.openBackupsModal = openBackupsModal;
    window.hideBackupsModal = hideBackupsModal;
    window.restoreSpecificBackup = restoreSpecificBackup;
    window.onBackupsListReceived = onBackupsListReceived;
    window.onConfigExported = onConfigExported;
    window.onConfigImported = onConfigImported;
    window.onThumbnailCacheCleared = onThumbnailCacheCleared;
    window.applyTheme = applyTheme;
    window.selectThemePreset = selectThemePreset;
    window.onThemeColorPick = onThemeColorPick;
    window.onThemeDepthInput = onThemeDepthInput;
    window.openSlotColorModal = openSlotColorModal;
    window.openWheelColorModal = openWheelColorModal;
    window.hideColorPickerModal = hideColorPickerModal;
    window.confirmTargetColor = confirmTargetColor;
    window.resetTargetColor = resetTargetColor;
    window.selectModalPresetColor = selectModalPresetColor;
    window.onModalColorInput = onModalColorInput;
}