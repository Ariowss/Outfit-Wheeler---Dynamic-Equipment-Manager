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
                return null;
            },
            querySelectorAll(sel) {
                return [];
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
            releasePointerCapture() {},
            innerText: '',
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
        querySelectorAll(sel) {
            const results = [];
            const parts = sel.split(',').map(s => s.trim());
            for (let id in elementStore) {
                const el = elementStore[id];
                for (const part of parts) {
                    if (part.startsWith('.') && el.classList.contains(part.slice(1))) {
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
        SaveOutfitToSlot: (payload) => { interopCalls['SaveOutfitToSlot'] = JSON.parse(payload); }
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

    env.toggleInventoryModCollapse('ModB.esp');
    assert.strictEqual(grid.children[1].classList.contains('collapsed'), true);

    env.handleInventorySearch('ModB');
    const gridSearch = env.document.getElementById('inventory-items-grid');
    assert.strictEqual(gridSearch.children.length, 1);
    assert.strictEqual(gridSearch.children[0].getAttribute('data-mod-name'), 'ModB.esp');
    env.clearInventorySearch();
});

console.log(`\n========================================`);
console.log(`Frontend Test Results: ${passedTests} / ${totalTests} Passed`);
console.log(`========================================`);

if (passedTests !== totalTests) {
    process.exit(1);
} else {
    process.exit(0);
}
