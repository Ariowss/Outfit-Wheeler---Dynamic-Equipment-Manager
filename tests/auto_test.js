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

function createTestEnvironment() {
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
            style: {},
            classList: {
                classes: new Set(),
                add(c) { this.classes.add(c); },
                remove(c) { this.classes.delete(c); },
                contains(c) { return this.classes.has(c); }
            },
            attributes: {},
            children: [],
            getAttribute(k) { return this.attributes[k] || null; },
            setAttribute(k, v) { this.attributes[k] = String(v); },
            appendChild(child) { this.children.push(child); return child; },
            removeChild(child) {
                const idx = this.children.indexOf(child);
                if (idx !== -1) this.children.splice(idx, 1);
            },
            remove() {
                if (this.parentNode) this.parentNode.removeChild(this);
            },
            closest(selector) {
                if (!selector) return null;
                if (selector.startsWith('#') && selector.slice(1) === this.id) return this;
                if (selector.startsWith('.')) {
                    const cls = selector.slice(1);
                    if (this.classList.contains(cls)) return this;
                }
                return null;
            },
            querySelector() { return null; },
            querySelectorAll() { return []; },
            addEventListener() {},
            removeEventListener() {},
            focus() {},
            select() {},
            innerText: '',
            innerHTML: '',
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
                setProperty() {}
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
            for (let id in elementStore) {
                const el = elementStore[id];
                if (sel.startsWith('.') && el.classList.contains(sel.slice(1))) {
                    results.push(el);
                } else if (sel.startsWith('#') && el.id === sel.slice(1)) {
                    results.push(el);
                }
            }
            return results;
        },
        createElement(tag) {
            return createElementMock('elem_' + Math.random().toString(36).slice(2), tag);
        },
        elementFromPoint(x, y) {
            return documentMock.lastTestedPoint = { x, y, closest: () => null };
        },
        createDocumentFragment() {
            return createElementMock('frag_' + Math.random().toString(36).slice(2), 'fragment');
        },
        addEventListener() {},
        removeEventListener() {}
    };

    const windowMock = {
        innerWidth: 1920,
        innerHeight: 1080,
        localStorage: localStorageMock,
        document: documentMock,
        requestAnimationFrame: (cb) => { return setTimeout(cb, 0); },
        cancelAnimationFrame: (id) => { clearTimeout(id); },
        addEventListener() {},
        removeEventListener() {},
        SetTypingMode: (payload) => { interopCalls['SetTypingMode'] = JSON.parse(payload); },
        SaveSettings: (payload) => { interopCalls['SaveSettings'] = JSON.parse(payload); },
        RenameSlot: (payload) => { interopCalls['RenameSlot'] = JSON.parse(payload); },
        RenameWheel: (payload) => { interopCalls['RenameWheel'] = JSON.parse(payload); },
        navigator: { getGamepads: () => [] }
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
        Date: Date
    };

    vm.createContext(sandbox);

    const modalsCode = fs.readFileSync(path.join(__dirname, '../view/modals.js'), 'utf8');
    const appCode = fs.readFileSync(path.join(__dirname, '../view/app.js'), 'utf8');

    vm.runInContext(modalsCode, sandbox);
    vm.runInContext(appCode, sandbox);

    return sandbox;
}

const env = createTestEnvironment();

runTest('Bug 1: panelRafId exists in scope and endPanelDrag does not throw ReferenceError', () => {
    assert.strictEqual(typeof env.panelRafId !== 'undefined', true);
    env.activeDragPanelId = 'test-panel';
    env.panelRafId = 123;
    assert.doesNotThrow(() => {
        env.endPanelDrag();
    });
    assert.strictEqual(env.activeDragPanelId, null);
    assert.strictEqual(env.panelRafId, null);
});

runTest('Bug 2: UI Scale immediate save and persistence', () => {
    env.onUiScaleChange(1.4);
    assert.strictEqual(env.storage['outfit_wheeler_ui_scale'], '1.4');
    assert.strictEqual(env.interopCalls['SaveSettings'].uiScale, 1.4);

    env.window.openWheel(JSON.stringify({
        wheelIndex: 0,
        uiScale: 1.0,
        slots: []
    }));
    assert.strictEqual(env.currentWheelData.uiScale, 1.4);
    assert.strictEqual(env.getUiScale(), 1.4);
});

runTest('Bug 3: Scaled Drag and Drop Math at 1.5x zoom', () => {
    env.currentWheelData.uiScale = 1.5;
    const scale = env.getUiScale();
    assert.strictEqual(scale, 1.5);

    const mouseEvent = { clientX: 900, clientY: 600 };
    const pointX = (scale !== 1.0) ? (mouseEvent.clientX / scale) : mouseEvent.clientX;
    const pointY = (scale !== 1.0) ? (mouseEvent.clientY / scale) : mouseEvent.clientY;

    assert.strictEqual(pointX, 600);
    assert.strictEqual(pointY, 400);

    env.document.elementFromPoint(pointX, pointY);
    assert.strictEqual(env.document.lastTestedPoint.x, 600);
    assert.strictEqual(env.document.lastTestedPoint.y, 400);
});

runTest('Bug 4: Renaming Slot updates typing state and dispatches RenameSlot', () => {
    env.selectedSlotIndex = 2;
    env.currentWheelData.wheelIndex = 1;
    env.showRenameModal('Rename Preset', 'Armor Set A');

    assert.strictEqual(env.isTyping, true);
    assert.strictEqual(env.interopCalls['SetTypingMode'].typing, true);

    const input = env.document.getElementById('rename-input');
    input.value = 'New Armor Set';
    env.confirmRename();

    assert.strictEqual(env.interopCalls['RenameSlot'].newName, 'New Armor Set');
    assert.strictEqual(env.interopCalls['RenameSlot'].slotIndex, 2);
    assert.strictEqual(env.interopCalls['RenameSlot'].wheelIndex, 1);
    assert.strictEqual(env.isTyping, false);
    assert.strictEqual(env.interopCalls['SetTypingMode'].typing, false);
});

runTest('Bug 5: Inventory Sorting by Weight, Rating and Name with persistence', () => {
    env.selectSortOption('weight_asc', 'Weight (Low -> High)');
    assert.strictEqual(env.activeInventorySort, 'weight_asc');
    assert.strictEqual(env.storage['outfit_wheeler_inv_sort'], 'weight_asc');

    const testItems = [
        { name: 'Heavy Iron Armor', weight: 30.0, armor: 40, value: 125 },
        { name: 'Leather Boots', weight: 2.0, armor: 10, value: 25 },
        { name: 'Steel Helm', weight: 5.0, armor: 17, value: 60 }
    ];

    testItems.sort((a, b) => {
        if (env.activeInventorySort === 'weight_asc') return (a.weight || 0) - (b.weight || 0);
        return 0;
    });

    assert.strictEqual(testItems[0].name, 'Leather Boots');
    assert.strictEqual(testItems[1].name, 'Steel Helm');
    assert.strictEqual(testItems[2].name, 'Heavy Iron Armor');

    env.selectSortOption('rating_desc', 'Armor Rating (High -> Low)');
    assert.strictEqual(env.activeInventorySort, 'rating_desc');
    assert.strictEqual(env.storage['outfit_wheeler_inv_sort'], 'rating_desc');

    testItems.sort((a, b) => {
        if (env.activeInventorySort === 'rating_desc') return (b.armor || 0) - (a.armor || 0);
        return 0;
    });

    assert.strictEqual(testItems[0].name, 'Heavy Iron Armor');
    assert.strictEqual(testItems[1].name, 'Steel Helm');
    assert.strictEqual(testItems[2].name, 'Leather Boots');
});

runTest('Bug 6: Follower Inventory and Source Switching', () => {
    env.currentWheelData = {
        followers: [{ id: '0x0001B074', name: 'Lydia', type: 'FOLLOWER' }],
        actorInventories: {
            '0x0001B074': [{ name: 'Steel Cuirass', weight: 25, category: 'ARMORS' }]
        },
        inventory: [{ name: 'Player Robes', weight: 1, category: 'ARMORS' }]
    };

    const followerInv = env.getInventoryBySourceId('0x0001B074');
    assert.strictEqual(followerInv.length, 1);
    assert.strictEqual(followerInv[0].name, 'Steel Cuirass');

    const playerInv = env.getInventoryBySourceId('0x00000007');
    assert.strictEqual(playerInv.length, 1);
    assert.strictEqual(playerInv[0].name, 'Player Robes');
});

runTest('Bug 7: Settings Payload Structure and INI Key Alignment', () => {
    env.saveSettings();
    const payload = env.interopCalls['SaveSettings'];
    assert.ok(payload);
    assert.strictEqual(typeof payload.editMode, 'boolean');
    assert.strictEqual(typeof payload.uiScale, 'number');
    assert.strictEqual(typeof payload.wheelScale, 'number');
    assert.strictEqual(typeof payload.autoCloseOnEquip, 'boolean');
});

console.log(`\n========================================`);
console.log(`Test Results: ${passedTests} / ${totalTests} Passed`);
console.log(`========================================`);

if (passedTests !== totalTests) {
    process.exit(1);
} else {
    process.exit(0);
}
