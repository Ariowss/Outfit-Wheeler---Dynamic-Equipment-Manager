var currentWheelData = {
    editMode: false,
    wheelIndex: 0,
    wheelName: "Wheel 1",
    totalWheels: 1,
    wheelScale: 1.0,
    wheelOffsetX: 0.0,
    activeTargetActorId: "0x00000007",
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
var gamepadHoverSlotIndex = null;

var isOrbitingCamera = false;
var lastCamMouseX = 0;
var lastCamMouseY = 0;

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
    applyModeLayout();
    loadPanelsLayout();
    initDraggablePanels();
    initResizablePanels();
    initCameraOrbitAndControls();
    initGlobalInputHandlers();
    initCarouselScrollHandlers();
});

function isEditModeActive() {
    return currentWheelData && currentWheelData.editMode === true;
}

function setTyping(isTyping) {
    if (typeof window.SetTypingMode === 'function') {
        window.SetTypingMode(JSON.stringify({ typing: isTyping }));
    }
}

window.openWheel = function(jsonStr) {
    try {
        currentWheelData = JSON.parse(jsonStr);
        activeWheelContextFilter = 'ALL';
        gamepadHoverSlotIndex = null;

        const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
        tabs.forEach(tab => {
            if (tab.getAttribute('data-tag') === 'ALL') tab.classList.add('active');
            else tab.classList.remove('active');
        });

        applyModeLayout();
        loadPanelsLayout();
        renderFollowersStrip();
        renderInventorySourceStrip();
        renderInventoryList();
        renderSlotsRail();
        renderDrawerDetails();
        if (typeof updateWheelUI === 'function') {
            updateWheelUI();
        }
    } catch (e) {
        console.error("Failed to parse wheel data:", e);
    }
};

window.setGamepadAngle = function(phiStr) {
    if (isEditModeActive()) return;
    const phi = parseFloat(phiStr);
    if (isNaN(phi)) return;

    const paths = document.querySelectorAll('.slice-path');
    const total = paths.length;
    if (total === 0) return;

    const sliceAngle = (2 * Math.PI) / total;
    let targetIdx = Math.floor(phi / sliceAngle);
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
    }
};

window.onGamepadButtonPress = function(btnCodeStr) {};

window.gamepadConfirm = function() {
    if (isEditModeActive()) return;
    const paths = document.querySelectorAll('.slice-path');
    if (gamepadHoverSlotIndex !== null && gamepadHoverSlotIndex < paths.length) {
        const targetPath = paths[gamepadHoverSlotIndex];
        if (targetPath) {
            const realIdx = parseInt(targetPath.getAttribute('data-slot-index'), 10);
            if (!isNaN(realIdx)) {
                equipOutfit(realIdx);
            }
        }
    }
};

window.gamepadCycleWheel = function(dirStr) {
    if (isEditModeActive()) return;
    changeWheel(parseInt(dirStr, 10));
};

window.gamepadCycleFilter = function(dirStr) {
    if (isEditModeActive()) return;
    const delta = parseInt(dirStr, 10) || 1;
    const tags = ['ALL', 'City', 'Home', 'Combat', 'Swim'];
    let idx = tags.indexOf(activeWheelContextFilter);
    if (idx === -1) idx = 0;
    idx = (idx + delta + tags.length) % tags.length;
    setWheelContextFilter(tags[idx]);
};

function applyModeLayout() {
    const wrapper = document.getElementById('ui-wrapper');
    const centerModeText = document.getElementById('mode-text');
    const gpBar = document.getElementById('gamepad-hints-bar');
    if (!wrapper) return;

    if (currentWheelData.editMode) {
        wrapper.classList.remove('play-mode-active');
        if (centerModeText) centerModeText.innerText = "EDIT MODE";
        if (gpBar) gpBar.style.display = 'none';
    } else {
        wrapper.classList.add('play-mode-active');
        if (centerModeText) centerModeText.innerText = "PLAY MODE";
        if (gpBar) {
            const hasGp = currentWheelData.gamepadConnected === true;
            gpBar.style.display = hasGp ? 'flex' : 'none';
        }
    }
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

function initGlobalInputHandlers() {
    document.addEventListener('wheel', (e) => {
        if (isEditModeActive()) return;
        if (e.target.closest('#settings-modal') || e.target.closest('#keyboard-modal') || e.target.closest('#gamepad-modal') || e.target.closest('.modal-card')) {
            return;
        }
        const now = Date.now();
        if (now - lastWheelScrollTimestamp < 150) return;
        lastWheelScrollTimestamp = now;

        if (currentWheelData) {
            const delta = e.deltaY < 0 ? -1 : 1;
            changeWheel(delta);
        }
    }, { passive: false });

    window.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' || e.code === 'Escape' || e.keyCode === 27) {
            e.preventDefault();
            e.stopPropagation();

            const modals = ['rename-modal', 'context-modal', 'settings-modal', 'keyboard-modal', 'gamepad-modal'];
            for (let id of modals) {
                const el = document.getElementById(id);
                if (el && el.style.display === 'flex') {
                    el.style.display = 'none';
                    setTyping(false);
                    return;
                }
            }

            const slotCtx = document.getElementById('slot-context-menu');
            const wheelCtx = document.getElementById('wheel-context-menu');
            if ((slotCtx && slotCtx.style.display === 'flex') || (wheelCtx && wheelCtx.style.display === 'flex')) {
                hideAllMenus();
                return;
            }

            closeWheel();
        }
    }, true);
}

function setWheelContextFilter(tag, e) {
    if (e) e.stopPropagation();
    activeWheelContextFilter = tag;

    const tabs = document.querySelectorAll('#filter-tabs-container .filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === tag) tab.classList.add('active');
        else tab.classList.remove('active');
    });

    if (typeof updateWheelUI === 'function') {
        updateWheelUI();
    }
}

function initCameraOrbitAndControls() {
    window.addEventListener('mousedown', (e) => {
        if (!isEditModeActive()) return;
        if (e.button !== 0) return;
        if (e.target.closest('.glass-panel') || e.target.closest('.fullscreen-modal-backdrop') || e.target.closest('#top-follower-bar') || e.target.closest('.context-menu-popup')) {
            return;
        }

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
        if (e.target.closest('.panel-body-scrollable') || e.target.closest('.slots-rail-list') || e.target.closest('.drawer-items-list') || e.target.closest('.modal-card') || e.target.closest('#top-follower-bar') || e.target.closest('.inv-source-carousel-wrapper')) {
            return;
        }

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

        const snapDist = 18;
        const panelW = panel.offsetWidth;
        const panelH = panel.offsetHeight;
        const screenW = window.innerWidth;
        const screenH = window.innerHeight;

        if (Math.abs(targetLeft - 16) < snapDist) targetLeft = 16;
        if (Math.abs(targetLeft + panelW - (screenW - 16)) < snapDist) targetLeft = screenW - panelW - 16;
        if (Math.abs(targetTop - 16) < snapDist) targetTop = 16;
        if (Math.abs(targetTop + panelH - (screenH - 16)) < snapDist) targetTop = screenH - panelH - 16;

        document.querySelectorAll('.draggable-panel').forEach(other => {
            if (other.id === activeDragPanelId || other.style.display === 'none') return;
            const oRect = other.getBoundingClientRect();

            if (Math.abs(targetLeft - (oRect.right + 12)) < snapDist) targetLeft = oRect.right + 12;
            if (Math.abs(targetLeft + panelW - (oRect.left - 12)) < snapDist) targetLeft = oRect.left - panelW - 12;
            if (Math.abs(targetLeft - oRect.left) < snapDist) targetLeft = oRect.left;
            if (Math.abs(targetTop - oRect.top) < snapDist) targetTop = oRect.top;
            if (Math.abs(targetTop - (oRect.bottom + 12)) < snapDist) targetTop = oRect.bottom + 12;
            if (Math.abs(targetTop + panelH - (oRect.top - 12)) < snapDist) targetTop = oRect.top - panelH - 12;
        });

        targetLeft = Math.max(8, Math.min(targetLeft, screenW - panelW - 8));
        targetTop = Math.max(8, Math.min(targetTop, screenH - panelH - 8));

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

        if (resizeDirection.includes('r')) {
            newW = Math.max(180, Math.min(panelStartWidth + dx, screenW - panelStartLeft - 10));
        }
        if (resizeDirection.includes('l')) {
            const maxDx = panelStartWidth - 180;
            const clampedDx = Math.min(maxDx, Math.max(-panelStartLeft + 10, dx));
            newW = panelStartWidth - clampedDx;
            newLeft = panelStartLeft + clampedDx;
        }
        if (resizeDirection.includes('b')) {
            newH = Math.max(150, Math.min(panelStartHeight + dy, screenH - panelStartTop - 10));
        }
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
        strip.innerHTML = `
            <div class="follower-tab-item active" onclick="selectFollower('0x00000007')">
                <div class="follower-avatar">D</div>
                <span class="follower-name">Player Character</span>
            </div>
        `;
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
        item.className = `follower-tab-item ${isActive ? 'active' : ''}`;
        item.innerHTML = `
            <div class="follower-avatar">${f.avatar || 'F'}</div>
            <span class="follower-name">${f.name}</span>
        `;
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
    sources.push({
        id: "0x00000007",
        type: "PLAYER",
        name: "🎒 Player"
    });

    const followers = currentWheelData.followers || [];
    followers.forEach(f => {
        const fID = (f.id || '').toUpperCase();
        if (fID !== "0X00000007" && fID !== "0X00000014") {
            sources.push({
                id: f.id,
                type: "FOLLOWER",
                name: `👤 ${f.name || 'Follower'}`
            });
        }
    });

    sources.push({
        id: "CLOUD",
        type: "CLOUD",
        name: "☁️ Cloud"
    });

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
        displayList = [
            sources[prevIdx],
            sources[activeIdx],
            sources[nextIdx]
        ];
    }

    strip.innerHTML = '';
    displayList.forEach(s => {
        const btn = document.createElement('button');
        const isActive = (s.id.toUpperCase() === activeInventorySource.toUpperCase());
        btn.className = `inv-source-chip ${isActive ? 'active' : ''}`;
        btn.innerHTML = `<span>${s.name}</span>`;
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
        if (srcUpper === 'CLOUD') {
            emptyMsg = 'Cloud Wardrobe Chest is empty.';
        }
        grid.innerHTML = `<div style="font-size:10.5px; color:#64748b; text-align:center; padding:24px;">${emptyMsg}</div>`;
        return;
    }

    items.forEach(item => {
        const card = document.createElement('div');
        card.className = `item-row-card ${item.equipped ? 'is-equipped-item' : ''}`;

        let iconClass = 'icon-armor';
        if (item.category === 'JEWELRY') iconClass = 'icon-jewelry';

        let badgeTagsHtml = '';
        if (item.source === 'CLOUD') badgeTagsHtml += '<span class="badge-tag" style="background:rgba(82,165,245,0.2); color:#52a5f5; border:1px solid #52a5f5;">CLOUD</span>';
        if (item.tags && item.tags.includes('enchanted')) badgeTagsHtml += '<span class="badge-tag tag-enchanted">ENCH</span>';
        if (item.tags && item.tags.includes('stolen')) badgeTagsHtml += '<span class="badge-tag tag-stolen">STOLEN</span>';

        let statPillText = `ARM ${item.armor || 0}`;

        card.innerHTML = `
            <div class="item-left-info">
                <div class="item-icon-box ${iconClass}">${item.icon || '🛡️'}</div>
                <div class="item-meta-titles">
                    <span class="item-name-text">${item.name}</span>
                    <span class="item-sub-desc">${item.type || 'Apparel'} • Wt ${Number(item.weight || 0).toFixed(1)} • ${item.value || 0}g</span>
                </div>
            </div>
            <div class="item-right-stats">
                <div class="item-badges-strip">${badgeTagsHtml}</div>
                <span class="item-stat-pill">${statPillText}</span>
            </div>
        `;

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
        card.className = `vertical-slot-card ${isSelected ? 'active' : ''} ${s.isEquipped ? 'is-equipped' : ''} ${s.isAddon ? 'is-addon' : ''}`;

        let pipHtml = '';
        if (s.isAddon) pipHtml = '<span class="slot-status-pip pip-addon">ADDON</span>';
        else if (s.isEquipped) pipHtml = '<span class="slot-status-pip pip-equipped">EQUIPPED</span>';

        let icon = s.isAddon ? '✨' : (s.hasOutfit ? '🛡️' : '➕');

        card.innerHTML = `
            <div class="slot-card-icon">${icon}</div>
            <div class="slot-card-texts">
                <span class="slot-card-name">${s.hasOutfit ? s.name : 'Empty Slot'}</span>
                <span class="slot-card-sub">Slot ${realIdx + 1} • ${s.count || 0} pcs</span>
            </div>
            ${pipHtml}
        `;

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