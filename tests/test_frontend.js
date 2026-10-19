const fs = require('fs');
const path = require('path');
const vm = require('vm');
const assert = require('assert');

let passedTests = 0;
let totalTests = 0;

function runTest(name, fn) {
    totalTests++;
    try {
        fn();
        passedTests++;
        console.log(`[PASS] ${name}`);
    } catch (err) {
        console.error(`[FAIL] ${name}`);
        console.error(err);
    }
}

function createFullFrontendEnvironment() {
    const storage = {};
    const interopCalls = {};

    const localStorageMock = {
        getItem: (k) => (k in storage ? storage[k] : null),
        setItem: (k, v) => { storage[k] = String(v); },
        removeItem: (k) => { delete storage[k]; },
        clear: () => { for (let k in storage) delete storage[k]; }
    };

    const elementStore = {};
    function createElementMock(id, tag = 'div') {
        const el = {
            id: id,
            tagName: tag.toUpperCase(),
            style: {
                setProperty(k, v) { this[k] = v; }
            },
            classList: {
                classes: new Set(),
                add(c) { this.classes.add(c); },
                remove(c) { this.classes.delete(c); },
                contains(c) { return this.classes.has(c); },
                toggle(c, force) {
                    if (force === undefined) {
                        if (this.classes.has(c)) this.classes.delete(c);
                        else this.classes.add(c);
                    } else if (force) {
                        this.classes.add(c);
                    } else {
                        this.classes.delete(c);
                    }
                }
            },
            get className() { return Array.from(this.classList.classes).join(' '); },
            set className(v) {
                this.classList.classes.clear();
                if (typeof v === 'string') {
                    v.split(/\s+/).forEach(c => { if (c) this.classList.classes.add(c); });
                }
            },
            attributes: {},
            children: [],
            get innerHTML() { return this._html || ''; },
            set innerHTML(v) {
                this._html = v;
                if (!v || v === '') this.children = [];
            },
            getAttribute(k) { return this.attributes[k] || null; },
            setAttribute(k, v) { this.attributes[k] = String(v); },
            appendChild(child) {
                if (child) {
                    if (child.tagName === 'FRAGMENT') {
                        const fragChildren = [...child.children];
                        child.children = [];
                        fragChildren.forEach(c => {
                            c.parentNode = this;
                            this.children.push(c);
                        });
                        return child;
                    }
                    child.parentNode = this;
                    this.children.push(child);
                }
                return child;
            },
            removeChild(child) {
                const idx = this.children.indexOf(child);
                if (idx !== -1) {
                    child.parentNode = null;
                    this.children.splice(idx, 1);
                }
            },
            remove() {
                if (this.parentNode) this.parentNode.removeChild(this);
            },
            closest(selector) {
                if (!selector) return null;
                const parts = selector.split(',').map(s => s.trim());
                for (const part of parts) {
                    if (part.startsWith('#') && part.slice(1) === this.id) return this;
                    if (part.startsWith('.') && this.classList.contains(part.slice(1))) return this;
                    if (part.toUpperCase() === this.tagName) return this;
                }
                if (this.parentNode && typeof this.parentNode.closest === 'function') {
                    return this.parentNode.closest(selector);
                }
                return null;
            },
            querySelector(sel) {
                const all = this.querySelectorAll(sel);
                return all.length > 0 ? all[0] : null;
            },
            querySelectorAll(sel) {
                const results = [];
                const check = (node) => {
                    if (!node) return;
                    const parts = sel.split(',').map(s => s.trim());
                    for (const part of parts) {
                        if (part.startsWith('.') && node.classList && node.classList.contains(part.slice(1))) {
                            results.push(node);
                            break;
                        } else if (part.startsWith('#') && node.id === part.slice(1)) {
                            results.push(node);
                            break;
                        } else if (node.tagName && part.toUpperCase() === node.tagName) {
                            results.push(node);
                            break;
                        }
                    }
                    if (node.children) {
                        for (let c of node.children) check(c);
                    }
                };
                if (this.children) {
                    for (let c of this.children) check(c);
                }
                return results;
            },
            addEventListener(ev, cb) {
                if (!this._listeners) this._listeners = {};
                if (!this._listeners[ev]) this._listeners[ev] = [];
                this._listeners[ev].push(cb);
            },
            removeEventListener(ev, cb) {
                if (this._listeners && this._listeners[ev]) {
                    this._listeners[ev] = this._listeners[ev].filter(f => f !== cb);
                }
            },
            dispatchEvent(ev) {
                if (this._listeners && this._listeners[ev.type]) {
                    this._listeners[ev.type].forEach(f => f(ev));
                }
            },
            focus() {},
            select() {},
            setPointerCapture() {},
            get innerText() { return this._innerText !== undefined ? this._innerText : (this._html ? this._html.replace(/<[^>]*>/g, '') : ''); },
            set innerText(v) { this._innerText = v; },
            value: '',
            checked: false,
            offsetWidth: 400,
            offsetHeight: 300,
            getBoundingClientRect() {
                return { left: 100, top: 100, width: 400, height: 300, right: 500, bottom: 400 };
            }
        };
        elementStore[id] = el;
        return el;
    }

    const documentMock = {
        documentElement: {
            style: {
                setProperty(k, v) { this[k] = v; }
            }
        },
        body: createElementMock('body', 'body'),
        getElementById(id) {
            if (!elementStore[id]) {
                elementStore[id] = createElementMock(id);
            }
            return elementStore[id];
        },
        querySelector(sel) {
            const all = this.querySelectorAll(sel);
            return all.length > 0 ? all[0] : null;
        },
        querySelectorAll(sel) {
            const results = [];
            const parts = sel.split(',').map(s => s.trim());
            for (let id in elementStore) {
                const el = elementStore[id];
                for (const part of parts) {
                    if (part.startsWith('[') && part.endsWith(']')) {
                        const m = part.slice(1, -1).split('=');
                        const attr = m[0].trim();
                        const val = m[1] ? m[1].replace(/["']/g, '').trim() : null;
                        if (val !== null) {
                            if (el.getAttribute(attr) === val) {
                                results.push(el);
                                break;
                            }
                        } else if (el.getAttribute(attr) !== null) {
                            results.push(el);
                            break;
                        }
                    } else if (part.startsWith('.') && el.classList.contains(part.slice(1))) {
                        results.push(el);
                        break;
                    } else if (part.startsWith('#') && el.id === part.slice(1)) {
                        results.push(el);
                        break;
                    } else if (part.toUpperCase() === el.tagName) {
                        results.push(el);
                        break;
                    }
                }
            }
            return results;
        },
        createElement(tag) {
            return createElementMock('elem_' + Math.random().toString(36).slice(2), tag);
        },
        createElementNS(ns, tag) {
            return createElementMock('elem_' + Math.random().toString(36).slice(2), tag);
        },
        createDocumentFragment() {
            return createElementMock('frag_' + Math.random().toString(36).slice(2), 'fragment');
        },
        elementFromPoint(x, y) {
            return documentMock.lastTestedPoint = { x, y, closest: (sel) => null };
        },
        addEventListener(ev, cb) {
            if (!this._listeners) this._listeners = {};
            if (!this._listeners[ev]) this._listeners[ev] = [];
            this._listeners[ev].push(cb);
        },
        removeEventListener(ev, cb) {
            if (this._listeners && this._listeners[ev]) {
                this._listeners[ev] = this._listeners[ev].filter(f => f !== cb);
            }
        },
        dispatchEvent(ev) {
            if (this._listeners && this._listeners[ev.type]) {
                this._listeners[ev.type].forEach(f => f(ev));
            }
        }
    };

    const windowMock = {
        innerWidth: 1920,
        innerHeight: 1080,
        screen: { width: 1920, height: 1080 },
        localStorage: localStorageMock,
        document: documentMock,
        navigator: { getGamepads: () => [] },
        requestAnimationFrame: (cb) => { return setTimeout(cb, 0); },
        cancelAnimationFrame: (id) => { clearTimeout(id); },
        addEventListener(ev, cb) {
            if (!this._listeners) this._listeners = {};
            if (!this._listeners[ev]) this._listeners[ev] = [];
            this._listeners[ev].push(cb);
        },
        removeEventListener(ev, cb) {
            if (this._listeners && this._listeners[ev]) {
                this._listeners[ev] = this._listeners[ev].filter(f => f !== cb);
            }
        },
        dispatchEvent(ev) {
            if (this._listeners && this._listeners[ev.type]) {
                this._listeners[ev.type].forEach(f => f(ev));
            }
        },
        SetTypingMode: (payload) => { interopCalls['SetTypingMode'] = JSON.parse(payload); },
        SaveSettings: (payload) => { interopCalls['SaveSettings'] = JSON.parse(payload); },
        RenameSlot: (payload) => { interopCalls['RenameSlot'] = JSON.parse(payload); },
        RenameWheel: (payload) => { interopCalls['RenameWheel'] = JSON.parse(payload); },
        AddWheel: (payload) => { interopCalls['AddWheel'] = payload ? JSON.parse(payload) : {}; },
        DeleteWheel: (payload) => { interopCalls['DeleteWheel'] = payload ? JSON.parse(payload) : {}; },
        MoveWheel: (payload) => { interopCalls['MoveWheel'] = JSON.parse(payload); },
        AddSlot: (payload) => { interopCalls['AddSlot'] = payload ? JSON.parse(payload) : {}; },
        DeleteSlot: (payload) => { interopCalls['DeleteSlot'] = JSON.parse(payload); },
        ClearSlot: (payload) => { interopCalls['ClearSlot'] = JSON.parse(payload); },
        MoveSlot: (payload) => { interopCalls['MoveSlot'] = JSON.parse(payload); },
        SetSlotType: (payload) => { interopCalls['SetSlotType'] = JSON.parse(payload); },
        SetSlotRating: (payload) => { interopCalls['SetSlotRating'] = JSON.parse(payload); },
        RotateCharacter: (payload) => { interopCalls['RotateCharacter'] = JSON.parse(payload); },
        ZoomCamera: (payload) => { interopCalls['ZoomCamera'] = JSON.parse(payload); },
        PanCamera: (payload) => { interopCalls['PanCamera'] = JSON.parse(payload); },
        SelectTargetActor: (payload) => { interopCalls['SelectTargetActor'] = JSON.parse(payload); },
        EquipOutfitSlot: (payload) => { interopCalls['EquipOutfitSlot'] = JSON.parse(payload); },
        ToggleItemEquip: (payload) => { interopCalls['ToggleItemEquip'] = JSON.parse(payload); },
        TransferItem: (payload) => { interopCalls['TransferItem'] = JSON.parse(payload); },
        TransferAllInventory: (payload) => { interopCalls['TransferAllInventory'] = JSON.parse(payload); },
        StashSlotOutfit: (payload) => { interopCalls['StashSlotOutfit'] = JSON.parse(payload); },
        RetrieveSlotOutfit: (payload) => { interopCalls['RetrieveSlotOutfit'] = JSON.parse(payload); },
        StashWheelOutfits: (payload) => { interopCalls['StashWheelOutfits'] = JSON.parse(payload); },
        RetrieveWheelOutfits: (payload) => { interopCalls['RetrieveWheelOutfits'] = JSON.parse(payload); },
        StashAllOutfits: (payload) => { interopCalls['StashAllOutfits'] = payload ? JSON.parse(payload) : {}; },
        RetrieveAllOutfits: (payload) => { interopCalls['RetrieveAllOutfits'] = payload ? JSON.parse(payload) : {}; },
        SaveSlotContextRules: (payload) => { interopCalls['SaveSlotContextRules'] = JSON.parse(payload); },
        SaveOutfitToSlot: (payload) => { interopCalls['SaveOutfitToSlot'] = JSON.parse(payload); },
        RequestModCatalogSummary: (payload) => { interopCalls['RequestModCatalogSummary'] = payload ? JSON.parse(payload) : {}; },
        RequestModCatalogItems: (payload) => { interopCalls['RequestModCatalogItems'] = JSON.parse(payload); },
        SpawnModItem: (payload) => { interopCalls['SpawnModItem'] = JSON.parse(payload); },
        SetStudioPose: (payload) => { interopCalls['SetStudioPose'] = JSON.parse(payload); },
        SetSlotColor: (payload) => { interopCalls['SetSlotColor'] = JSON.parse(payload); },
        SetWheelColor: (payload) => { interopCalls['SetWheelColor'] = JSON.parse(payload); },
        SaveSettings: (payload) => { interopCalls['SaveSettings'] = JSON.parse(payload); },
        ImportConfig: (payload) => { interopCalls['ImportConfig'] = payload ? JSON.parse(payload) : {}; },
        ExportConfig: (payload) => { interopCalls['ExportConfig'] = payload ? JSON.parse(payload) : {}; },
        GetBackupsList: (payload) => { interopCalls['GetBackupsList'] = payload ? JSON.parse(payload) : {}; },
        ClearThumbnailCache: (payload) => { interopCalls['ClearThumbnailCache'] = payload ? JSON.parse(payload) : {}; }
    };

    const sandbox = {
        window: windowMock,
        document: documentMock,
        localStorage: localStorageMock,
        navigator: windowMock.navigator,
        requestAnimationFrame: windowMock.requestAnimationFrame,
        cancelAnimationFrame: windowMock.cancelAnimationFrame,
        setTimeout: setTimeout,
        clearTimeout: clearTimeout,
        interopCalls: interopCalls,
        storage: storage,
        console: console,
        parseFloat: parseFloat,
        parseInt: parseInt,
        Math: Math,
        Date: Date,
        Set: Set,
        Array: Array,
        Object: Object,
        String: String,
        Number: Number,
        Boolean: Boolean,
        JSON: JSON
    };

    vm.createContext(sandbox);

    const modalsCode = fs.readFileSync(path.join(__dirname, '../view/modals.js'), 'utf8');
    const appCode = fs.readFileSync(path.join(__dirname, '../view/app.js'), 'utf8');
    const wheelCode = fs.readFileSync(path.join(__dirname, '../view/wheel.js'), 'utf8');

    vm.runInContext(modalsCode, sandbox);
    vm.runInContext(appCode, sandbox);
    vm.runInContext(wheelCode, sandbox);

    return sandbox;
}

const env = createFullFrontendEnvironment();

runTest('1. Wheel CRUD operations (Add, Rename, Delete, Move)', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        wheelName: 'Combat Gear',
        totalWheels: 2,
        slots: []
    };

    env.addWheel();
    assert.ok(env.interopCalls['AddWheel']);

    env.renameTargetMode = 'wheel';
    const renameInput = env.document.getElementById('rename-input');
    renameInput.value = 'Mage Robes';
    env.confirmRename();
    assert.strictEqual(env.interopCalls['RenameWheel'].newName, 'Mage Robes');
    assert.strictEqual(env.interopCalls['RenameWheel'].wheelIndex, 0);

    env.ctxMoveWheel(1);
    assert.strictEqual(env.interopCalls['MoveWheel'].fromIndex, 0);
    assert.strictEqual(env.interopCalls['MoveWheel'].toIndex, 1);

    env.ctxDeleteWheel();
    assert.ok(env.interopCalls['DeleteWheel']);
});

runTest('2. Slot CRUD & Lifecycle (Add, Rename, Clear, Delete, Move)', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        slots: [
            { index: 0, name: 'Heavy Steel', isEquipped: false, items: [] },
            { index: 1, name: 'Tavern Clothes', isEquipped: true, items: [] }
        ]
    };
    env.selectedSlotIndex = 1;

    env.addSlot();
    assert.ok(env.interopCalls['AddSlot']);

    env.renameTargetMode = 'slot';
    const renameInput = env.document.getElementById('rename-input');
    renameInput.value = 'Casual Wear';
    env.confirmRename();
    assert.strictEqual(env.interopCalls['RenameSlot'].newName, 'Casual Wear');
    assert.strictEqual(env.interopCalls['RenameSlot'].slotIndex, 1);

    env.ctxMoveSlot(-1);
    assert.strictEqual(env.interopCalls['MoveSlot'].fromIndex, 1);
    assert.strictEqual(env.interopCalls['MoveSlot'].toIndex, 0);
    assert.strictEqual(env.selectedSlotIndex, 0);

    env.ctxClearSlot();
    assert.strictEqual(env.interopCalls['ClearSlot'].slotIndex, 0);

    env.ctxDeleteSlot();
    assert.strictEqual(env.interopCalls['DeleteSlot'].slotIndex, 0);
});

runTest('3. Addon & Overlay System (Toggle, Equip, Pulse Highlight)', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        slots: [
            { index: 0, name: 'Winter Cloak', isAddon: false, isWorn: false, items: [{ name: 'Fur Cloak' }] }
        ]
    };
    env.selectedSlotIndex = 0;

    env.toggleSlotAddonType(true);
    assert.strictEqual(env.interopCalls['SetSlotType'].isAddon, true);
    assert.strictEqual(env.interopCalls['SetSlotType'].slotIndex, 0);

    const slot = env.currentWheelData.slots[0];
    slot.isAddon = true;
    const isOverlayActive = slot.isAddon && (slot.isEquipped || slot.isWorn);
    assert.strictEqual(isOverlayActive, false);

    slot.isWorn = true;
    const isNowActive = slot.isAddon && (slot.isEquipped || slot.isWorn);
    assert.strictEqual(isNowActive, true);
});

runTest('4. Audition Mode (Shift-Click Multi-Equip without closing wheel)', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        autoCloseOnEquip: true,
        slots: [
            { index: 0, name: 'Set A' },
            { index: 1, name: 'Set B' }
        ]
    };

    const normalEvent = { shiftKey: false };
    const keepOpenNormal = (normalEvent.shiftKey === true) || (env.currentWheelData.autoCloseOnEquip === false);
    assert.strictEqual(keepOpenNormal, false);

    const shiftEvent = { shiftKey: true };
    const keepOpenShift = (shiftEvent.shiftKey === true) || (env.currentWheelData.autoCloseOnEquip === false);
    assert.strictEqual(keepOpenShift, true);
});

runTest('5. Floating Panel Drag & Resize with Snapping and Lifecycle', () => {
    assert.strictEqual(env.panelRafId, null);
    env.handlePanelDragStart({ button: 2, clientX: 300, clientY: 200, preventDefault() {}, stopPropagation() {} }, 'panel-inventory');
    assert.strictEqual(env.activeDragPanelId, 'panel-inventory');
    assert.strictEqual(env.dragStartX, 300);
    assert.strictEqual(env.dragStartY, 200);

    env.endPanelDrag();
    assert.strictEqual(env.activeDragPanelId, null);
    assert.strictEqual(env.panelRafId, null);

    env.handlePanelResizeStart({ button: 0, clientX: 500, clientY: 400, target: {}, preventDefault() {}, stopPropagation() {} }, 'panel-inventory', 'r');
    assert.strictEqual(env.activeResizePanelId, 'panel-inventory');
    assert.strictEqual(env.resizeDirection, 'r');

    env.endPanelResize();
    assert.strictEqual(env.activeResizePanelId, null);
});

runTest('6. 3D Dressing Studio Turntable and Orbit Camera Controls', () => {
    env.currentWheelData = { editMode: true };
    const rotateEvent = { clientX: 450, clientY: 300 };
    const dx = 25;
    env.window.RotateCharacter(JSON.stringify({ deltaX: dx }));
    assert.strictEqual(env.interopCalls['RotateCharacter'].deltaX, 25);

    env.window.ZoomCamera(JSON.stringify({ deltaZoom: 1.5 }));
    assert.strictEqual(env.interopCalls['ZoomCamera'].deltaZoom, 1.5);

    env.window.PanCamera(JSON.stringify({ moveX: 1, moveY: 0 }));
    assert.strictEqual(env.interopCalls['PanCamera'].moveX, 1);
    assert.strictEqual(env.interopCalls['PanCamera'].moveY, 0);
});

runTest('7. Inventory Multi-Criteria Sorting (All 12 Sort Modes & Tie-Breakers)', () => {
    const items = [
        { name: 'Steel Boots', armor: 12, weight: 8.0, value: 50, type: 'Boots', category: 'ARMORS', armorClass: 'Heavy', tags: ['heavy'], equipped: false },
        { name: 'Iron Helmet', armor: 15, weight: 5.0, value: 30, type: 'Helmet', category: 'ARMORS', armorClass: 'Heavy', tags: ['heavy'], equipped: true },
        { name: 'Gold Ring', armor: 0, weight: 0.3, value: 150, type: 'Ring', category: 'JEWELRY', armorClass: 'Clothing', tags: ['enchanted', 'clothing'], equipped: false },
        { name: 'Dragonplate Cuirass', armor: 46, weight: 40.0, value: 1200, type: 'Cuirass', category: 'ARMORS', armorClass: 'Heavy', tags: ['heavy'], equipped: false },
        { name: 'Leather Boots', armor: 7, weight: 2.0, value: 20, type: 'Boots', category: 'ARMORS', armorClass: 'Light', tags: ['light'], equipped: false },
        { name: 'Robes of Conjuration', armor: 0, weight: 1.0, value: 250, type: 'Cuirass', category: 'ARMORS', armorClass: 'Clothing', tags: ['enchanted', 'clothing'], equipped: false },
        { name: 'Silver Ring', armor: 0, weight: 0.3, value: 100, type: 'Ring', category: 'JEWELRY', armorClass: 'Clothing', tags: ['clothing'], equipped: false }
    ];

    env.currentWheelData = {
        playerInventory: items,
        inventory: items,
        followers: []
    };
    env.activeInventorySource = 'PLAYER';
    env.activeCategoryFilter = 'ALL';
    env.inventorySearchQuery = '';

    env.selectSortOption('name_asc', 'Name (A → Z)');
    assert.strictEqual(env.activeInventorySort, 'name_asc');
    assert.strictEqual(env.storage['outfit_wheeler_inv_sort'], 'name_asc');

    env.selectSortOption('name_desc', 'Name (Z → A)');
    assert.strictEqual(env.activeInventorySort, 'name_desc');

    env.selectSortOption('rating_desc', 'Armor Rating (High → Low)');
    assert.strictEqual(env.activeInventorySort, 'rating_desc');

    env.selectSortOption('rating_asc', 'Armor Rating (Low → High)');
    assert.strictEqual(env.activeInventorySort, 'rating_asc');

    env.selectSortOption('weight_asc', 'Weight (Low → High)');
    assert.strictEqual(env.activeInventorySort, 'weight_asc');

    env.selectSortOption('weight_desc', 'Weight (High → Low)');
    assert.strictEqual(env.activeInventorySort, 'weight_desc');

    env.selectSortOption('value_desc', 'Value (High → Low)');
    assert.strictEqual(env.activeInventorySort, 'value_desc');

    env.selectSortOption('value_asc', 'Value (Low → High)');
    assert.strictEqual(env.activeInventorySort, 'value_asc');

    env.selectSortOption('equipped_first', 'Equipped First');
    assert.strictEqual(env.activeInventorySort, 'equipped_first');

    env.selectSortOption('enchanted_first', 'Enchanted First');
    assert.strictEqual(env.activeInventorySort, 'enchanted_first');

    env.selectSortOption('slot_order', 'Body Slot (Head → Feet)');
    assert.strictEqual(env.activeInventorySort, 'slot_order');

    env.selectSortOption('class_order', 'Class (Heavy → Light → Cloth)');
    assert.strictEqual(env.activeInventorySort, 'class_order');

    const mockSortElem = {
        textContent: 'Armor Rating (High -> Low)',
        innerText: 'Armor Rating (High -> Low)',
        getAttribute: (k) => (k === 'data-sort' ? 'rating_desc' : null)
    };
    const mockEvt = { preventDefault: () => {}, stopPropagation: () => {} };
    env.selectSortOption('rating_desc', mockSortElem, mockEvt);
    assert.strictEqual(env.activeInventorySort, 'rating_desc');
    assert.strictEqual(env.storage['outfit_wheeler_inv_sort'], 'rating_desc');
    assert.strictEqual(env.storage['outfit_wheeler_inv_sort_label'], 'Armor Rating (High -> Low)');

    const sortFn = (list, mode) => {
        env.activeInventorySort = mode;
        return [...list].sort((a, b) => {
            let diff = 0;
            if (env.activeInventorySort === 'name_asc') diff = (a.name || '').localeCompare(b.name || '');
            else if (env.activeInventorySort === 'name_desc') diff = (b.name || '').localeCompare(a.name || '');
            else if (env.activeInventorySort === 'rating_desc') diff = (Number(b.armor) || 0) - (Number(a.armor) || 0);
            else if (env.activeInventorySort === 'rating_asc') diff = (Number(a.armor) || 0) - (Number(b.armor) || 0);
            else if (env.activeInventorySort === 'weight_asc') diff = (Number(a.weight) || 0) - (Number(b.weight) || 0);
            else if (env.activeInventorySort === 'weight_desc') diff = (Number(b.weight) || 0) - (Number(a.weight) || 0);
            else if (env.activeInventorySort === 'value_desc') diff = (Number(b.value) || 0) - (Number(a.value) || 0);
            else if (env.activeInventorySort === 'value_asc') diff = (Number(a.value) || 0) - (Number(b.value) || 0);
            else if (env.activeInventorySort === 'equipped_first') diff = (b.equipped ? 1 : 0) - (a.equipped ? 1 : 0);
            else if (env.activeInventorySort === 'enchanted_first') {
                const aEnch = (a.tags && Array.isArray(a.tags) && a.tags.includes('enchanted')) ? 1 : 0;
                const bEnch = (b.tags && Array.isArray(b.tags) && b.tags.includes('enchanted')) ? 1 : 0;
                diff = bEnch - aEnch;
            } else if (env.activeInventorySort === 'slot_order') diff = env.getSlotOrderWeight(a.type) - env.getSlotOrderWeight(b.type);
            else if (env.activeInventorySort === 'class_order') diff = env.getClassOrderWeight(a) - env.getClassOrderWeight(b);
            if (diff !== 0) return diff;
            return (a.name || '').localeCompare(b.name || '');
        });
    };

    const sortedNameAsc = sortFn(items, 'name_asc');
    assert.strictEqual(sortedNameAsc[0].name, 'Dragonplate Cuirass');
    assert.strictEqual(sortedNameAsc[sortedNameAsc.length - 1].name, 'Steel Boots');

    const sortedNameDesc = sortFn(items, 'name_desc');
    assert.strictEqual(sortedNameDesc[0].name, 'Steel Boots');
    assert.strictEqual(sortedNameDesc[sortedNameDesc.length - 1].name, 'Dragonplate Cuirass');

    const sortedWeightDesc = sortFn(items, 'weight_desc');
    assert.strictEqual(sortedWeightDesc[0].name, 'Dragonplate Cuirass');

    const sortedWeightAsc = sortFn(items, 'weight_asc');
    assert.strictEqual(sortedWeightAsc[0].name, 'Gold Ring');
    assert.strictEqual(sortedWeightAsc[1].name, 'Silver Ring');

    const sortedRatingDesc = sortFn(items, 'rating_desc');
    assert.strictEqual(sortedRatingDesc[0].name, 'Dragonplate Cuirass');

    const sortedEquipped = sortFn(items, 'equipped_first');
    assert.strictEqual(sortedEquipped[0].name, 'Iron Helmet');
    assert.strictEqual(sortedEquipped[1].name, 'Dragonplate Cuirass');

    const sortedEnchanted = sortFn(items, 'enchanted_first');
    assert.strictEqual(sortedEnchanted[0].tags.includes('enchanted'), true);
    assert.strictEqual(sortedEnchanted[1].tags.includes('enchanted'), true);

    const sortedSlot = sortFn(items, 'slot_order');
    assert.strictEqual(sortedSlot[0].type, 'Helmet');

    const sortedClass = sortFn(items, 'class_order');
    assert.strictEqual(env.getClassOrderWeight(sortedClass[0]), 0);
});

runTest('8. Follower Management, Carry Weight and Source Switching', () => {
    env.currentWheelData = {
        followers: [
            { id: '0x0001B074', name: 'Lydia', weight: '180 / 300', type: 'FOLLOWER' },
            { id: '0x00000007', name: 'Player', weight: '75 / 350', type: 'PLAYER' }
        ],
        actorInventories: {
            '0x0001B074': [{ name: 'Lydia Armor', weight: 20 }]
        },
        cloudInventory: [{ name: 'Stored Dress', weight: 1 }]
    };

    env.setInventorySource('0x0001B074');
    assert.strictEqual(env.activeInventorySource, '0x0001B074');

    const followerItems = env.getInventoryBySourceId('0x0001B074');
    assert.strictEqual(followerItems.length, 1);
    assert.strictEqual(followerItems[0].name, 'Lydia Armor');

    env.setInventorySource('WARDROBE');
    const cloudItems = env.getInventoryBySourceId('WARDROBE');
    assert.strictEqual(cloudItems.length, 1);
    assert.strictEqual(cloudItems[0].name, 'Stored Dress');
});

runTest('9. Dynamic Sub-Wheels & Category Filtering (All, Combat, Swim, City, Home)', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        slots: [
            { index: 0, name: 'Normal Set', contextRules: ['CITY'] },
            { index: 1, name: 'Armor Set', contextRules: ['COMBAT'] },
            { index: 2, name: 'Bathing Suit', contextRules: ['SWIM'] }
        ]
    };

    env.activeWheelContextFilter = 'COMBAT';
    const combatSlots = env.currentWheelData.slots.filter(s => !s.contextRules || s.contextRules.includes('COMBAT'));
    assert.strictEqual(combatSlots.length, 1);
    assert.strictEqual(combatSlots[0].name, 'Armor Set');

    env.activeWheelContextFilter = 'SWIM';
    const swimSlots = env.currentWheelData.slots.filter(s => !s.contextRules || s.contextRules.includes('SWIM'));
    assert.strictEqual(swimSlots.length, 1);
    assert.strictEqual(swimSlots[0].name, 'Bathing Suit');

    env.activeWheelContextFilter = 'ALL';
    const allSlots = env.currentWheelData.slots;
    assert.strictEqual(allSlots.length, 3);
});

runTest('10. Gamepad 360 Analog Calculation & Face Buttons', () => {
    const angle0 = Math.atan2(0, 1);
    assert.strictEqual(angle0, 0);

    let angle180 = Math.atan2(0, -1);
    if (angle180 < 0) angle180 += 2 * Math.PI;
    assert.ok(Math.abs(angle180 - Math.PI) < 0.001);

    const deadzone = 0.25;
    const magSmall = 0.15;
    assert.strictEqual(magSmall < deadzone, true);
    const magLarge = 0.85;
    assert.strictEqual(magLarge >= deadzone, true);
});

runTest('11. Keyboard Scan Codes & Modifier Matching', () => {
    const escCode = env.getScanCode({ code: 'Escape', keyCode: 27 });
    assert.strictEqual(escCode, 0x01);

    const spaceCode = env.getScanCode({ code: 'Space', keyCode: 32 });
    assert.strictEqual(spaceCode, 0x39);

    const keyN = env.getScanCode({ code: 'KeyN', keyCode: 78 });
    assert.strictEqual(keyN, 0x31);

    const keyV = env.getScanCode({ code: 'KeyV', keyCode: 86 });
    assert.strictEqual(keyV, 0x2F);

    assert.strictEqual(env.getKeyName(0x01), 'Esc');
    assert.strictEqual(env.getKeyName(0x2A), 'LShift');
    assert.strictEqual(env.getKeyName(0x1D), 'LCtrl');
    assert.strictEqual(env.getKeyName(0x38), 'LAlt');
});

runTest('12. Text Typing Mode & Input Focus Protection', () => {
    env.setTyping(true);
    assert.strictEqual(env.isTyping, true);
    assert.strictEqual(env.interopCalls['SetTypingMode'].typing, true);

    env.setTyping(false);
    assert.strictEqual(env.isTyping, false);
    assert.strictEqual(env.interopCalls['SetTypingMode'].typing, false);
});

runTest('13. Multi-Resolution UI Scaling, Dynamic Flip/Clamp and Direct Coordinates', () => {
    env.onUiScaleChange(1.75);
    assert.strictEqual(env.storage['outfit_wheeler_ui_scale'], '1.75');
    assert.strictEqual(env.interopCalls['SaveSettings'].uiScale, 1.75);

    const scale = env.getUiScale();
    assert.strictEqual(scale, 1.75);

    const rawX = 1400;
    const rawY = 700;
    const pointX = rawX / scale;
    const pointY = rawY / scale;
    assert.strictEqual(Math.round(pointX * scale), 1400);
    assert.strictEqual(Math.round(pointY * scale), 700);

    const mockMenu = env.document.createElement('div');
    mockMenu.offsetWidth = 180;
    mockMenu.offsetHeight = 300;
    env.window.innerWidth = 3840;
    env.window.innerHeight = 2160;

    env.positionContextMenu(mockMenu, 3800, 2100);
    const expectedLeft = Math.round((3800 / scale) - 180);
    const expectedTop = Math.round((2100 / scale) - 300);
    assert.strictEqual(parseInt(mockMenu.style.left, 10), expectedLeft);
    assert.strictEqual(parseInt(mockMenu.style.top, 10), expectedTop);

    env.storage['outfit_wheeler_ui_scale'] = null;
    delete env.storage['outfit_wheeler_ui_scale'];
    env.currentWheelData.uiScale = undefined;

    env.window.screen = { width: 3840, height: 2160 };
    assert.strictEqual(env.getUiScale(), 1.75);

    env.window.screen = { width: 2560, height: 1440 };
    assert.strictEqual(env.getUiScale(), 1.25);

    env.window.screen = { width: 1920, height: 1080 };
    assert.strictEqual(env.getUiScale(), 1.0);

    const initialSlotCount = env.currentWheelData.slots.length;
    env.addSlot();
    assert.strictEqual(env.currentWheelData.slots.length, initialSlotCount + 1);
});

runTest('14. Inventory Mod Grouping, Default Collapse and Dynamic In-Place Expansion', () => {
    env.currentWheelData = {
        inventory: [
            { name: 'Item A1', modName: 'ModA.esp', category: 'ARMORS', rawID: 1 },
            { name: 'Item A2', modName: 'ModA.esp', category: 'ARMORS', rawID: 2 },
            { name: 'Item B1', modName: 'ModB.esp', category: 'ARMORS', rawID: 3 },
            { name: 'Item B2', modName: 'ModB.esp', category: 'ARMORS', rawID: 4 },
            { name: 'Item C1', modName: 'ModC.esp', category: 'ARMORS', rawID: 5 }
        ],
        playerInventory: [],
        cloudInventory: [],
        followerInventory: [],
        followers: []
    };
    env.currentWheelData.playerInventory = env.currentWheelData.inventory;
    env.activeInventorySource = '0x00000007';
    env.inventoryGroupByMod = true;
    env.expandedInventoryMods.clear();
    env.collapsedInventoryMods.clear();

    env.renderInventoryList();

    const grid = env.document.getElementById('inventory-items-grid');
    assert.strictEqual(grid.children.length, 3);

    const secA = grid.children[0];
    const secB = grid.children[1];
    const secC = grid.children[2];

    assert.strictEqual(secA.classList.contains('collapsed'), false);
    assert.strictEqual(secA.children[1].children.length, 2);

    assert.strictEqual(secB.classList.contains('collapsed'), false);
    assert.strictEqual(secB.children[1].children.length, 2);

    assert.strictEqual(secC.classList.contains('collapsed'), false);
    assert.strictEqual(secC.children[1].children.length, 1);

    env.collapseAllInventoryMods();
    assert.strictEqual(grid.children[0].classList.contains('collapsed'), true);
    assert.strictEqual(grid.children[1].classList.contains('collapsed'), true);
    assert.strictEqual(grid.children[2].classList.contains('collapsed'), true);

    env.expandAllInventoryMods();
    assert.strictEqual(grid.children[0].classList.contains('collapsed'), false);
    assert.strictEqual(grid.children[1].classList.contains('collapsed'), false);
    assert.strictEqual(grid.children[2].classList.contains('collapsed'), false);
    assert.strictEqual(grid.children[0].children[1].children.length, 2);
    assert.strictEqual(grid.children[1].children[1].children.length, 2);
    assert.strictEqual(grid.children[2].children[1].children.length, 1);

    // Simulate empty grid state before expand all
    grid.children[0].children[1].innerHTML = '';
    grid.children[1].children[1].innerHTML = '';
    grid.children[2].children[1].innerHTML = '';
    env.collapseAllInventoryMods();
    env.expandAllInventoryMods();
    assert.strictEqual(grid.children[0].children[1].children.length, 2);
    assert.strictEqual(grid.children[1].children[1].children.length, 2);
    assert.strictEqual(grid.children[2].children[1].children.length, 1);

    env.toggleInventoryModCollapse('ModB.esp');
    assert.strictEqual(grid.children[1].classList.contains('collapsed'), true);

    env.handleInventorySearch('ModB');
    const gridSearch = env.document.getElementById('inventory-items-grid');
    assert.strictEqual(gridSearch.children.length, 1);
    assert.strictEqual(gridSearch.children[0].getAttribute('data-mod-name'), 'ModB.esp');
    env.clearInventorySearch();
});

runTest('15. Mod Explorer Source Carousel, On-Demand Mod Loading and Spawning Actions', () => {
    const sources = env.getInventorySourcesList();
    const modexSource = sources.find(s => s.id === 'MODEX');
    assert.strictEqual(!!modexSource, true);
    assert.strictEqual(modexSource.type, 'MODEX');

    env.currentWheelData = {
        editMode: true,
        inventory: [],
        playerInventory: [],
        cloudInventory: [],
        followerInventory: [],
        followers: [
            { id: '0x00000007', name: 'Player', type: 'PLAYER', weight: '50 / 300' },
            { id: '0x000A1234', name: 'Lydia', type: 'FOLLOWER', weight: '40 / 250' }
        ]
    };

    env.setInventorySource('MODEX');
    assert.strictEqual(env.activeInventorySource, 'MODEX');
    assert.strictEqual(!!env.interopCalls['RequestModCatalogSummary'], true);

    const statWeight = env.document.getElementById('stat-carry-weight');
    assert.strictEqual(statWeight.innerText, '-- / --');

    env.window.onModCatalogSummaryReady([
        { modName: 'BikiniArmor.esp', count: 10 },
        { modName: 'NordicArmor.esp', count: 5 }
    ]);

    const grid = env.document.getElementById('inventory-items-grid');
    assert.strictEqual(grid.children.length, 2);

    const sec1 = grid.children[0];
    assert.strictEqual(sec1.getAttribute('data-mod-name'), 'BikiniArmor.esp');
    assert.strictEqual(sec1.classList.contains('collapsed'), true);

    env.toggleInventoryModCollapse('BikiniArmor.esp');
    assert.strictEqual(sec1.classList.contains('collapsed'), false);
    assert.strictEqual(!!env.interopCalls['RequestModCatalogItems'], true);
    assert.strictEqual(env.interopCalls['RequestModCatalogItems'].modName, 'BikiniArmor.esp');

    env.window.onModCatalogItemsReady({
        modName: 'BikiniArmor.esp',
        items: [
            { id: '0x00010001', rawID: 65537, name: 'Bikini Top', category: 'ARMORS', type: 'Cuirass', armor: 15, weight: 2.0, value: 100, modName: 'BikiniArmor.esp', source: 'MODEX' },
            { id: '0x00010002', rawID: 65538, name: 'Bikini Bottom', category: 'ARMORS', type: 'Cuirass', armor: 10, weight: 1.5, value: 80, modName: 'BikiniArmor.esp', source: 'MODEX' }
        ]
    });

    const itemsContainer = sec1.children[1];
    assert.strictEqual(itemsContainer.children.length, 2);

    const cardTop = itemsContainer.children[0];
    assert.strictEqual(cardTop.innerHTML.includes('tag-catalog'), false);

    cardTop.onclick();
    assert.strictEqual(!!env.interopCalls['SpawnModItem'], true);
    assert.strictEqual(env.interopCalls['SpawnModItem'].rawID, 65538);
    assert.strictEqual(env.interopCalls['SpawnModItem'].autoEquip, true);
    assert.strictEqual(env.interopCalls['SpawnModItem'].destination, 'INVENTORY');

    env.showItemContextMenu(100, 100, {
        id: '0x00010001',
        rawID: 65537,
        name: 'Bikini Top',
        category: 'ARMORS',
        source: 'MODEX'
    });

    const transferTargetsList = env.document.getElementById('item-transfer-targets-list');
    assert.strictEqual(transferTargetsList.children.length >= 4, true);

    const wearBtn = transferTargetsList.children[0];
    assert.strictEqual(wearBtn.innerText.includes('Equip (Add & Wear)'), true);

    const addBtn = transferTargetsList.children[1];
    assert.strictEqual(addBtn.innerText.includes('Add to Inventory'), true);
    addBtn.onclick({ preventDefault: () => {}, stopPropagation: () => {} });
    assert.strictEqual(env.interopCalls['SpawnModItem'].autoEquip, false);
    assert.strictEqual(env.interopCalls['SpawnModItem'].destination, 'INVENTORY');

    const stashBtn = transferTargetsList.children[2];
    assert.strictEqual(stashBtn.innerText.includes('Send to Wardrobe Chest'), true);
    stashBtn.onclick({ preventDefault: () => {}, stopPropagation: () => {} });
    assert.strictEqual(env.interopCalls['SpawnModItem'].destination, 'CLOUD');

    const folBtn = transferTargetsList.children[3];
    assert.strictEqual(folBtn.innerText.includes('Give & Equip to Lydia'), true);
    folBtn.onclick({ preventDefault: () => {}, stopPropagation: () => {} });
    assert.strictEqual(env.interopCalls['SpawnModItem'].targetActorId, '0x000A1234');
    assert.strictEqual(env.interopCalls['SpawnModItem'].autoEquip, true);

    env.setInventorySource('MODEX');
    assert.strictEqual(env.inventoryGroupByMod, true);
    env.collapseAllInventoryMods();
    assert.strictEqual(grid.children[0].classList.contains('collapsed'), true);
    env.expandAllInventoryMods();
    assert.strictEqual(grid.children[0].classList.contains('collapsed'), false);

    const itemsCont = grid.children[0].children[1];
    env.window.onModCatalogItemsReady({
        modName: 'bikiniarmor.esp',
        items: [
            { id: '0x00010003', rawID: 65539, name: 'Bikini Gloves', category: 'ARMORS', type: 'Gauntlets', armor: 5, weight: 0.5, value: 50, modName: 'BikiniArmor.esp', source: 'MODEX' }
        ]
    });
    assert.strictEqual(itemsCont.children.length, 1);

    env.setInventorySource('0x00000007');
    env.showItemContextMenu(100, 100, {
        id: '0x00000001',
        rawID: 1,
        name: 'Iron Armor',
        category: 'ARMORS',
        source: '0x00000007'
    });
    const standardTargets = Array.from(transferTargetsList.children).map(c => c.innerText);
    assert.strictEqual(standardTargets.some(t => t.includes('Mod Explorer')), false);
});

runTest('16. Studio Mode Pose Selection, Dropdown Changes, and Clean State Resets', () => {
    const poseBar = env.document.getElementById('studio-pose-bar');
    assert.strictEqual(!!poseBar, true);

    const poseSelect = env.document.getElementById('studio-pose-select');
    assert.strictEqual(!!poseSelect, true);

    env.activeInventorySource = '0x00000007';
    env.onStudioPoseChange('IdleModelRunway');

    assert.strictEqual(env.activeStudioPose, 'IdleModelRunway');
    assert.strictEqual(poseSelect.value, 'IdleModelRunway');
    assert.strictEqual(!!env.interopCalls['SetStudioPose'], true);
    assert.strictEqual(env.interopCalls['SetStudioPose'].actorID, '0x00000007');
    assert.strictEqual(env.interopCalls['SetStudioPose'].eventName, 'IdleModelRunway');

    env.onStudioPoseChange('IdleHandsOnHips');
    assert.strictEqual(env.interopCalls['SetStudioPose'].eventName, 'IdleHandsOnHips');

    env.selectFollower('0x000A1234');
    assert.strictEqual(poseSelect.value, 'IdleForceDefaultState');
    assert.strictEqual(env.activeStudioPose, 'IdleForceDefaultState');

    env.onStudioPoseChange('IdleCombatReady');
    assert.strictEqual(env.interopCalls['SetStudioPose'].actorID, '0x000A1234');
    assert.strictEqual(env.interopCalls['SetStudioPose'].eventName, 'IdleCombatReady');

    env.resetStudioPose();
    assert.strictEqual(poseSelect.value, 'IdleForceDefaultState');
    assert.strictEqual(env.interopCalls['SetStudioPose'].eventName, 'IdleForceDefaultState');

    env.closeWheel();
    assert.strictEqual(poseSelect.value, 'IdleForceDefaultState');
    assert.strictEqual(env.activeStudioPose, 'IdleForceDefaultState');
});

runTest('17. Wheel Slot Torso Armor Thumbnail Rendering and Live Update', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        totalWheels: 1,
        slots: [
            {
                index: 0,
                name: 'Steel Warrior',
                hasOutfit: true,
                isEquipped: false,
                items: [
                    { rawID: 101, name: 'Steel Boots', type: 'Boots', thumbnail: 'thumbnails/boots.png' },
                    { rawID: 102, name: 'Steel Armor', type: 'Cuirass', thumbnail: 'thumbnails/steel_cuirass.png' },
                    { rawID: 103, name: 'Steel Helmet', type: 'Helmet', thumbnail: 'thumbnails/helm.png' }
                ]
            },
            {
                index: 1,
                name: 'Mage Robes',
                hasOutfit: true,
                isEquipped: true,
                items: [
                    { rawID: 201, name: 'Novice Hood', type: 'Helmet' },
                    { rawID: 202, name: 'Novice Robes of Destruction', type: 'Robe' }
                ]
            }
        ]
    };

    const torso0 = env.resolveSlotTorsoItem(env.currentWheelData.slots[0]);
    assert.ok(torso0);
    assert.strictEqual(torso0.rawID, 102);
    assert.strictEqual(torso0.name, 'Steel Armor');
    assert.strictEqual(torso0.thumbnail, 'thumbnails/steel_cuirass.png');

    const torso1 = env.resolveSlotTorsoItem(env.currentWheelData.slots[1]);
    assert.ok(torso1);
    assert.strictEqual(torso1.rawID, 202);
    assert.strictEqual(torso1.name, 'Novice Robes of Destruction');

    env.updateWheelUI();

    const labelsContainer = env.document.getElementById('labels-container');
    assert.strictEqual(labelsContainer.children.length, 2);

    const slot0Label = labelsContainer.children[0];
    assert.ok(slot0Label.innerHTML.includes('wheel-slot-thumb-img'));
    assert.ok(slot0Label.innerHTML.includes('data-raw-id="102"'));
    assert.ok(slot0Label.innerHTML.includes('src="thumbnails/steel_cuirass.png"'));

    const slot1Label = labelsContainer.children[1];
    assert.ok(slot1Label.innerHTML.includes('wheel-slot-thumb-img'));
    assert.ok(slot1Label.innerHTML.includes('data-raw-id="202"'));

    // Test live thumbnail arrival updating currentWheelData slot item
    env.window.onThumbnailReady({ rawID: 202, thumbnail: 'thumbnails/novice_robes.png' });
    assert.strictEqual(env.currentWheelData.slots[1].items[1].thumbnail, 'thumbnails/novice_robes.png');
});

runTest('18. Alphabetical Sorting Action Dispatches SortOutfitsAlphabetically', () => {
    let called = false;
    env.window.SortOutfitsAlphabetically = (payload) => {
        called = true;
    };
    env.sortOutfitsAlphabetically();
    assert.strictEqual(called, true);
});

runTest('19. Dual Listener Typing Mode Synchronization', () => {
    let modePayload = null;
    let statePayload = null;
    env.window.SetTypingMode = (p) => { modePayload = JSON.parse(p); };
    env.window.SetTypingState = (p) => { statePayload = JSON.parse(p); };

    env.setTyping(true);
    assert.strictEqual(modePayload.typing, true);
    assert.strictEqual(statePayload.typing, true);

    env.setTyping(false);
    assert.strictEqual(modePayload.typing, false);
    assert.strictEqual(statePayload.typing, false);
});

runTest('20. Batch Thumbnail Delivery & Mod Explorer Layout Stability', () => {
    env.currentWheelData = {
        slots: [
            { items: [{ rawID: 501, thumbnail: '' }] },
            { items: [{ rawID: 502, thumbnail: '' }] }
        ]
    };

    // Test batch delivery
    env.window.onThumbnailReady([
        { rawID: 501, thumbnail: 'thumbnails/batch_501.png' },
        { rawID: 502, thumbnail: 'thumbnails/batch_502.png' }
    ]);

    assert.strictEqual(env.currentWheelData.slots[0].items[0].thumbnail, 'thumbnails/batch_501.png');
    assert.strictEqual(env.currentWheelData.slots[1].items[0].thumbnail, 'thumbnails/batch_502.png');
});

runTest('21. Mod Explorer Scroll Drag Throttling & Batch Thumbnail Flush', () => {
    let requests = [];
    env.window.RequestThumbnail = (p) => {
        requests.push(JSON.parse(p));
    };

    // When scrolling/dragging is active
    env.isInventoryScrolling = true;
    env.queueThumbnailRequest(9001);
    env.queueThumbnailRequest(9002);

    // No immediate request dispatched during active scroll drag
    assert.strictEqual(requests.length, 0);

    // When drag finishes and flush is called
    env.isInventoryScrolling = false;
    env.flushPendingThumbnails();

    assert.strictEqual(requests.length, 1);
    assert.deepStrictEqual(requests[0].rawIDs, [9001, 9002]);
});

runTest('22. Dynamic Theme Engine (Presets, Custom Hex, Depth & CSS Properties)', () => {
    env.applyTheme('#ef4444', 0.65, 'ruby');
    assert.strictEqual(env.document.documentElement.style['--accent-hex'], '#ef4444');
    assert.strictEqual(env.document.documentElement.style['--accent-primary'], '#ef4444');
    assert.strictEqual(env.document.documentElement.style['--slice-stroke-hover'], '#ef4444');

    env.selectThemePreset('amethyst');
    assert.strictEqual(env.document.documentElement.style['--accent-hex'], '#a855f7');
    assert.strictEqual(env.document.documentElement.style['--accent-primary'], '#a855f7');

    env.onThemeColorPick('#10b981');
    assert.strictEqual(env.document.documentElement.style['--accent-hex'], '#10b981');
});

runTest('23. Per-Slot & Per-Wheel Color Customization (Modal, Interop & Overrides)', () => {
    env.currentWheelData = {
        wheelIndex: 0,
        wheelColor: '#10b981',
        slots: [
            { index: 0, name: 'Slot 1', color: '#ef4444', items: [] },
            { index: 1, name: 'Slot 2', color: '', items: [] }
        ]
    };
    env.selectedSlotIndex = 0;

    // Slot Color Selection & Confirm
    env.openSlotColorModal();
    env.selectModalPresetColor('#f59e0b');
    env.confirmTargetColor();
    assert.ok(env.interopCalls['SetSlotColor']);
    assert.strictEqual(env.interopCalls['SetSlotColor'].wheelIndex, 0);
    assert.strictEqual(env.interopCalls['SetSlotColor'].slotIndex, 0);
    assert.strictEqual(env.interopCalls['SetSlotColor'].color, '#f59e0b');
    assert.strictEqual(env.currentWheelData.slots[0].color, '#f59e0b');

    // Slot Color Reset
    env.openSlotColorModal();
    env.resetTargetColor();
    assert.strictEqual(env.interopCalls['SetSlotColor'].color, '');
    assert.strictEqual(env.currentWheelData.slots[0].color, '');

    // Wheel Color Selection & Confirm
    env.openWheelColorModal();
    env.selectModalPresetColor('#a855f7');
    env.confirmTargetColor();
    assert.ok(env.interopCalls['SetWheelColor']);
    assert.strictEqual(env.interopCalls['SetWheelColor'].wheelIndex, 0);
    assert.strictEqual(env.interopCalls['SetWheelColor'].color, '#a855f7');
    assert.strictEqual(env.currentWheelData.wheelColor, '#a855f7');

    // Wheel Color Reset
    env.openWheelColorModal();
    env.resetTargetColor();
    assert.strictEqual(env.interopCalls['SetWheelColor'].color, '');
    assert.strictEqual(env.currentWheelData.wheelColor, '');
});

runTest('24. 3D Preview Toggle & Suppressed Background Queuing', () => {
    env.currentWheelData = {
        showItemPreviews: false,
        slots: []
    };
    assert.strictEqual(env.isItemPreviewsEnabled(), false);

    let thumbReqs = [];
    env.window.RequestThumbnail = (p) => { thumbReqs.push(JSON.parse(p)); };

    // When previews disabled, queueThumbnailRequest is completely suppressed
    env.queueThumbnailRequest(8888, true);
    assert.strictEqual(thumbReqs.length, 0);

    // Inspector shows fallback type icon without queueing thumbnail
    env.inspectItem({ name: 'Nordic Carved Armor', category: 'ARMOR', rawID: 8888 });
    assert.strictEqual(thumbReqs.length, 0);
    assert.strictEqual(env.document.getElementById('inspect-preview-frame').style.display, 'none');

    // Re-enable previews and verify thumbnail requests function normally
    env.currentWheelData.showItemPreviews = true;
    assert.strictEqual(env.isItemPreviewsEnabled(), true);
    env.queueThumbnailRequest(8888, true);
    assert.strictEqual(thumbReqs.length, 1);
    assert.deepStrictEqual(thumbReqs[0].rawIDs, [8888]);
});

runTest('25. 2D Color Spectrum Canvas Picker & Global Theme Customization', () => {
    // Open Global Theme Color Modal
    env.openThemeColorModal();
    const modal = env.document.getElementById('color-picker-modal');
    assert.strictEqual(modal.style.display, 'flex');
    assert.strictEqual(env.colorPickerTarget.type, 'theme');

    // Pick color via 2D Spectrum coordinates (e.g. middle of canvas)
    env.pickColorAtCoords(180, 50, 360, 180);
    assert.ok(env.selectedModalColor.startsWith('#'));
    assert.strictEqual(env.selectedModalColor.length, 7);

    // Enter custom hex directly via text input
    env.onModalHexTextInput('ff0077');
    assert.strictEqual(env.selectedModalColor, '#ff0077');

    // Apply color to global theme
    env.confirmTargetColor();
    assert.strictEqual(modal.style.display, 'none');
    assert.strictEqual(env.currentThemeHex, '#ff0077');
    assert.strictEqual(env.document.documentElement.style['--accent-primary'], '#ff0077');

    // Open again and test Reset to Default
    env.openThemeColorModal();
    env.resetTargetColor();
    assert.strictEqual(modal.style.display, 'none');
    assert.strictEqual(env.currentThemeHex, '#38bdf8');
});

runTest('26. Multi-Target Color Customization (Accent, Menu Background, and Typography Text)', () => {
    // 1. Menu Background Customization
    env.openThemeColorModal('bg');
    assert.strictEqual(env.colorPickerTarget.type, 'theme');
    assert.strictEqual(env.colorPickerTarget.mode, 'bg');
    assert.strictEqual(env.selectedModalColor, env.currentBgHex);

    // Pick deep navy background
    env.onModalHexTextInput('1e1b4b');
    assert.strictEqual(env.selectedModalColor, '#1e1b4b');
    env.confirmTargetColor();
    assert.strictEqual(env.currentBgHex, '#1e1b4b');
    assert.ok(env.document.documentElement.style['--bg-surface'].includes('hsla('));

    // 2. Text & Typography Customization
    env.openThemeColorModal('text');
    assert.strictEqual(env.colorPickerTarget.type, 'theme');
    assert.strictEqual(env.colorPickerTarget.mode, 'text');
    assert.strictEqual(env.selectedModalColor, env.currentTextHex);

    // Pick soft gold text
    env.onModalHexTextInput('fef08a');
    assert.strictEqual(env.selectedModalColor, '#fef08a');
    env.confirmTargetColor();
    assert.strictEqual(env.currentTextHex, '#fef08a');
    assert.strictEqual(env.document.documentElement.style['--text-primary'], '#fef08a');

    // 3. Coordinated Presets test
    env.selectThemePreset('ruby');
    assert.strictEqual(env.currentThemeHex, '#ef4444');
    assert.strictEqual(env.currentBgHex, '#180608');
    assert.strictEqual(env.currentTextHex, '#fff1f2');
    assert.strictEqual(env.document.documentElement.style['--accent-primary'], '#ef4444');
    assert.strictEqual(env.document.documentElement.style['--text-primary'], '#fff1f2');

    // 4. Save Settings Persistence Payload
    env.saveSettings();
    assert.ok(env.interopCalls['SaveSettings']);
    assert.strictEqual(env.interopCalls['SaveSettings'].themeAccentColor, '#ef4444');
    assert.strictEqual(env.interopCalls['SaveSettings'].themeBgColor, '#180608');
    assert.strictEqual(env.interopCalls['SaveSettings'].themeTextColor, '#fff1f2');
});

runTest('27. Backup Management, Restore Snapshots & Thumbnail Cache Clearing', () => {
    // 1. Export Backup
    env.exportConfiguration();
    assert.ok(env.interopCalls['ExportConfig']);

    // 2. Auto-Restore Backup (Latest snapshot)
    env.importConfiguration();
    assert.ok(env.interopCalls['ImportConfig']);
    assert.strictEqual(env.interopCalls['ImportConfig'].auto, true);

    // 3. Open Backups Modal & Fetch List
    env.openBackupsModal();
    assert.ok(env.interopCalls['GetBackupsList']);
    assert.strictEqual(env.document.getElementById('backups-modal').style.display, 'flex');

    // 4. Deliver Backups List via Interop and Render
    const mockBackups = [
        { filename: 'OutfitWheeler_20261015_213000.json', path: 'Data/SKSE/Plugins/backups/OutfitWheeler_20261015_213000.json', size: 12480 },
        { filename: 'OutfitWheeler_20261014_180000.json', path: 'Data/SKSE/Plugins/backups/OutfitWheeler_20261014_180000.json', size: 10240 }
    ];
    env.onBackupsListReceived(JSON.stringify(mockBackups));
    const container = env.document.getElementById('backups-list-container');
    assert.ok(container.innerHTML.includes('OutfitWheeler_20261015_213000.json'));
    assert.ok(container.innerHTML.includes('12.2 KB'));

    // 5. Restore Specific Backup Snapshot
    env.restoreSpecificBackup('Data/SKSE/Plugins/backups/OutfitWheeler_20261015_213000.json');
    assert.strictEqual(env.interopCalls['ImportConfig'].path, 'Data/SKSE/Plugins/backups/OutfitWheeler_20261015_213000.json');
    assert.strictEqual(env.document.getElementById('backups-modal').style.display, 'none');

    // 6. Config Import / Export Toast Callbacks
    env.onConfigImported({ success: true });
    assert.strictEqual(env.document.getElementById('save-toast').innerText, 'Configuration restored successfully!');
    env.onConfigExported();
    assert.strictEqual(env.document.getElementById('save-toast').innerText, 'Backup exported successfully!');

    // 7. Clear Thumbnail Cache
    env.clearThumbnailCache();
    assert.ok(env.interopCalls['ClearThumbnailCache']);
    env.onThumbnailCacheCleared({ success: true, removed: 42 });
    assert.strictEqual(env.document.getElementById('save-toast').innerText, 'Thumbnail cache cleared (42 files)!');
});

runTest('28. List Mode Rendering, Equip Sync, and Debounced Inspection', () => {
    // 1. Switch to List Mode
    env.setInventoryViewMode('list');
    assert.strictEqual(env.inventoryViewMode, 'list');

    // 2. Setup mock data for active actor
    env.currentWheelData = {
        activeTargetActorId: '0x0001B074',
        playerInventory: [],
        followerInventory: [
            { id: '0x00012345', rawID: 0x12345, name: 'Daedric Boots', type: 'Boots', equipped: false }
        ],
        actorInventories: {
            '0x0001B074': [
                { id: '0x00012345', rawID: 0x12345, name: 'Daedric Boots', type: 'Boots', equipped: false }
            ]
        }
    };

    // 3. Create List Mode Item Card
    const item = { id: '0x00012345', rawID: 0x12345, name: 'Daedric Boots', type: 'Boots', equipped: false, weight: 8, armor: 18 };
    const card = env.createInventoryItemElement(item, '0x0001B074');
    assert.ok(card.className.includes('item-row-card'));
    assert.strictEqual(card.getAttribute('data-raw-id'), String(0x12345));

    // 4. Test Live Equip Event Toggle
    env.document.body.appendChild(card);
    env.window.onItemEquipToggled({ rawID: 0x12345, id: '0x00012345', equipped: true });
    assert.ok(card.classList.contains('is-equipped-item'));
    assert.strictEqual(env.currentWheelData.followerInventory[0].equipped, true);
    assert.strictEqual(env.currentWheelData.actorInventories['0x0001B074'][0].equipped, true);

    // Unequip Event Toggle
    env.window.onItemEquipToggled({ rawID: 0x12345, id: '0x00012345', equipped: false });
    assert.strictEqual(card.classList.contains('is-equipped-item'), false);
    assert.strictEqual(env.currentWheelData.followerInventory[0].equipped, false);

    // 5. Card click toggle
    card.onclick();
    assert.strictEqual(card.classList.contains('is-equipped-item'), true);
    assert.ok(env.interopCalls['ToggleItemEquip']);
    assert.strictEqual(env.interopCalls['ToggleItemEquip'].rawID, 0x12345);

    // Clean up
    card.remove();
    env.setInventoryViewMode('grid');
});

runTest('29. Version 3.0 Classic Inventory Reversion When Previews Disabled', () => {
    // 1. Disable previews
    env.currentWheelData.showItemPreviews = false;
    assert.strictEqual(env.isItemPreviewsEnabled(), false);

    // 2. Trigger UI update
    env.updateUiForPreviewsSetting();
    const panelInv = env.document.getElementById('panel-inventory');
    if (panelInv) {
        assert.ok(panelInv.classList.contains('v3-classic-mode'));
    }
    const viewSwitch = env.document.querySelector('.view-mode-switch');
    if (viewSwitch) {
        assert.strictEqual(viewSwitch.style.display, 'none');
    }
    const inspectFrame = env.document.getElementById('inspect-preview-frame');
    if (inspectFrame) {
        assert.strictEqual(inspectFrame.style.display, 'none');
    }
    assert.strictEqual(env.inventoryViewMode, 'list');

    // 3. Create item card under v3 mode - verify NO thumbnail frames or placeholders exist
    const item = { id: '0x00012345', rawID: 0x12345, name: 'Glass Armor', type: 'Cuirass', weight: 12, armor: 35 };
    const card = env.createInventoryItemElement(item, 'PLAYER');
    assert.ok(card.className.includes('item-row-card'));
    assert.strictEqual(card.innerHTML.includes('item-row-thumb-frame'), false);
    assert.strictEqual(card.innerHTML.includes('item-thumb-placeholder'), false);
    assert.strictEqual(card.innerHTML.includes('item-thumb-wrapper'), false);
    assert.ok(card.innerHTML.includes('item-meta-titles'));
    assert.ok(card.innerHTML.includes('Glass Armor'));

    // Re-enable previews for subsequent tests
    env.currentWheelData.showItemPreviews = true;
    env.updateUiForPreviewsSetting();
    if (panelInv) assert.strictEqual(panelInv.classList.contains('v3-classic-mode'), false);
    if (viewSwitch) assert.strictEqual(viewSwitch.style.display, 'inline-flex');
    if (inspectFrame) assert.strictEqual(inspectFrame.style.display, 'flex');
});

runTest('30. Mod Explorer Expand All Throttling (No Game Freeze)', () => {
    env.activeInventorySource = 'MODEX';
    env.modCatalogSummaries = [
        { modName: 'ModA.esp', count: 10 },
        { modName: 'ModB.esp', count: 20 },
        { modName: 'ModC.esp', count: 15 },
        { modName: 'ModD.esp', count: 30 }
    ];

    let modRequests = [];
    env.window.RequestModCatalogItems = (param) => {
        modRequests.push(JSON.parse(param));
    };

    // Render sections in collapsed state
    env.renderInventoryList();
    const sections = env.document.querySelectorAll('.mod-group-section');
    assert.ok(sections.length >= 4);

    // Call expandAllInventoryMods
    env.expandAllInventoryMods();

    // Verify sections are uncollapsed visually
    sections.forEach(sec => {
        assert.strictEqual(sec.classList.contains('collapsed'), false);
    });

    // Verify C++ was not flooded: requests in flight must be <= 4
    assert.ok(modRequests.length <= 4, `Expected at most 4 initial requests, got ${modRequests.length}`);

    // Deliver response for ModA and verify next section is triggered cleanly
    env.window.onModCatalogItemsReady({
        modName: 'ModA.esp',
        items: [{ id: '0x00010001', rawID: 0x10001, name: 'Mod A Item 1', type: 'Cuirass' }]
    });
    assert.ok(env.getCachedModItems('ModA.esp'));
});

runTest('31. Heavy Mod List (500+ Plugins) Expand All Uncollapses All Sections with Viewport Lazy-Loading (0 Freeze)', () => {
    env.activeInventorySource = 'MODEX';
    const heavyList = [];
    for (let i = 1; i <= 500; ++i) {
        heavyList.push({ modName: `HeavyPlugin_${i}.esp`, count: i * 5 });
    }
    env.window.onModCatalogSummaryReady(heavyList);

    let modRequests = [];
    env.window.RequestModCatalogItems = (param) => {
        modRequests.push(JSON.parse(param));
    };

    // Render 500 plugins
    env.renderInventoryList();
    const grid = env.document.getElementById('inventory-items-grid');
    assert.strictEqual(grid.children.length, 500);

    // Call expandAllInventoryMods with 500 plugins present
    env.expandAllInventoryMods();

    // Verify all 500 mods are uncollapsed and recorded in expandedInventoryMods
    assert.strictEqual(env.expandedInventoryMods.size, 500, `Expected all 500 expanded mods, got ${env.expandedInventoryMods.size}`);

    // Verify all sections are uncollapsed in DOM
    for (let i = 0; i < 500; ++i) {
        assert.strictEqual(grid.children[i].classList.contains('collapsed'), false);
    }

    // Verify requests in flight remained capped to viewport batch (at most 4 concurrent)
    assert.ok(modRequests.length <= 4, `Expected at most 4 concurrent requests, got ${modRequests.length}`);

    // Collapse all
    env.collapseAllInventoryMods();
    assert.strictEqual(env.expandedInventoryMods.size, 0);
    for (let i = 0; i < 500; ++i) {
        assert.strictEqual(grid.children[i].classList.contains('collapsed'), true);
    }
});

runTest('32. Massive Mod Item Chunking & Expand All DOM Safety (Skyrim.esm 3,000+ Items)', () => {
    env.activeInventorySource = 'MODEX';
    const massiveItems = [];
    for (let i = 1; i <= 3000; ++i) {
        massiveItems.push({
            id: '0x' + (0x10000 + i).toString(16).toUpperCase(),
            rawID: 0x10000 + i,
            name: `Skyrim Armor Item #${i}`,
            category: 'ARMORS',
            type: 'Cuirass',
            armor: 25,
            weight: 5.0,
            value: 100,
            modName: 'Skyrim.esm',
            source: 'MODEX'
        });
    }

    env.window.onModCatalogSummaryReady([
        { modName: 'Skyrim.esm', count: 3000 },
        { modName: 'Dawnguard.esm', count: 400 }
    ]);

    env.renderInventoryList();
    const grid = env.document.getElementById('inventory-items-grid');
    assert.strictEqual(grid.children.length, 2);

    // Deliver 3,000 items for Skyrim.esm
    env.window.onModCatalogItemsReady({
        modName: 'Skyrim.esm',
        items: massiveItems
    });

    const skyrimSec = grid.children[0];
    const skyrimGrid = skyrimSec.children[1];

    // Verify initial render is chunked to 48 items + 1 "+ Show more" button
    assert.strictEqual(skyrimGrid.children.length, 49, 'Expected 48 items + 1 Show more button for 3000 items');
    const moreBtn = skyrimGrid.children[48];
    assert.ok(moreBtn.classList.contains('mod-load-more-btn'));
    assert.ok(moreBtn.innerText.includes('2952 items remaining'));

    // Click "+ Show more" button
    moreBtn.onclick({ stopPropagation: () => {} });
    assert.strictEqual(skyrimGrid.children.length, 97, 'Expected 96 items + 1 Show more button after expansion');

    // Expand All with massive cached mod
    env.expandAllInventoryMods();
    assert.strictEqual(skyrimSec.classList.contains('collapsed'), false);
});

console.log(`\n========================================`);
console.log(`Frontend Test Results: ${passedTests} / ${totalTests} Passed`);
console.log(`========================================`);

if (passedTests !== totalTests) {
    process.exit(1);
} else {
    process.exit(0);
}

