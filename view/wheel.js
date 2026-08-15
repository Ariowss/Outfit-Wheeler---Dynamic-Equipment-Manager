var lastEquipCallTimestamp = 0;

function describeArc(x, y, innerRadius, outerRadius, startAngle, endAngle) {
    const x1_out = x + outerRadius * Math.cos(startAngle);
    const y1_out = y + outerRadius * Math.sin(startAngle);
    const x2_out = x + outerRadius * Math.cos(endAngle);
    const y2_out = y + outerRadius * Math.sin(endAngle);

    const x1_in = x + innerRadius * Math.cos(endAngle);
    const y1_in = y + innerRadius * Math.sin(endAngle);
    const x2_in = x + innerRadius * Math.cos(startAngle);
    const y2_in = y + innerRadius * Math.sin(startAngle);

    const largeArcFlag = (endAngle - startAngle) > Math.PI ? 1 : 0;

    return [
        "M " + x1_out + " " + y1_out,
        "A " + outerRadius + " " + outerRadius + " 0 " + largeArcFlag + " 1 " + x2_out + " " + y2_out,
        "L " + x1_in + " " + y1_in,
        "A " + innerRadius + " " + innerRadius + " 0 " + largeArcFlag + " 0 " + x2_in + " " + y2_in,
        "Z"
    ].join(" ");
}

function updateWheelUI() {
    window.requestAnimationFrame(() => {
        const container = document.getElementById('wheel-container');
        const svg = document.getElementById('wheel-svg');
        const labelsContainer = document.getElementById('labels-container');
        const modeText = document.getElementById('mode-text');
        const wheelName = document.getElementById('wheel-name');
        const editActions = document.getElementById('edit-actions');

        if (!svg || !labelsContainer || !modeText || !wheelName || !container) return;

        const scale = currentWheelData.wheelScale || 1.0;
        const offsetX = currentWheelData.wheelOffsetX || 0.0;
        container.style.transform = 'translateX(' + offsetX + 'px) scale(' + scale + ')';

        const isEdit = isEditModeActive();
        modeText.innerText = isEdit ? "EDIT MODE" : "PLAY MODE";
        modeText.style.color = isEdit ? "#6c829c" : "#cddbe8";

        if (activeCategoryFilter !== 'ALL') {
            wheelName.innerText = activeCategoryFilter + ' View';
        } else {
            wheelName.innerText = currentWheelData.wheelName || ('Wheel ' + (currentWheelData.wheelIndex + 1));
        }

        if (editActions) {
            editActions.style.display = isEdit ? "flex" : "none";
        }

        svg.innerHTML = '';
        labelsContainer.innerHTML = '';

        const rawSlots = currentWheelData.slots || [];
        let displaySlots = [];

        if (activeCategoryFilter === 'ALL') {
            rawSlots.forEach((s, idx) => {
                const realIdx = s.index !== undefined ? s.index : idx;
                const hasOutfit = s && s.hasOutfit && (s.count === undefined || s.count > 0 || (s.items && s.items.length > 0));
                if (isEdit || hasOutfit) {
                    displaySlots.push({ ...s, realIndex: realIdx });
                }
            });
        } else {
            const filterTag = activeCategoryFilter.toLowerCase();
            rawSlots.forEach((s, idx) => {
                const realIdx = s.index !== undefined ? s.index : idx;
                const hasOutfit = s && s.hasOutfit && (s.count === undefined || s.count > 0 || (s.items && s.items.length > 0));
                if (s && s.hasOutfit && s.contextTag) {
                    const tagLower = s.contextTag.toLowerCase();
                    if (tagLower.includes(filterTag)) {
                        if (isEdit || hasOutfit) {
                            displaySlots.push({ ...s, realIndex: realIdx });
                        }
                    }
                }
            });
        }

        updateGamepadHintsVisibility();

        const total = displaySlots.length;
        if (total === 0) return;

        const cx = 290;
        const cy = 290;
        const rIn = 95;
        const rOut = 245;
        const rMid = (rIn + rOut) / 2;

        const sliceAngle = (2 * Math.PI) / total;

        const svgFragment = document.createDocumentFragment();
        const labelFragment = document.createDocumentFragment();

        displaySlots.forEach((slot, index) => {
            let startAngle, endAngle;
            if (total === 1) {
                startAngle = -Math.PI / 2;
                endAngle = startAngle + (2 * Math.PI) - 0.001;
            } else {
                const gapAngle = 0.02;
                startAngle = (index * sliceAngle) - (Math.PI / 2) + (gapAngle / 2);
                endAngle = ((index + 1) * sliceAngle) - (Math.PI / 2) - (gapAngle / 2);
            }

            const midAngle = (startAngle + endAngle) / 2;
            const hasOutfit = slot.hasOutfit;
            const realSlotIndex = slot.realIndex;
            const isEquipped = hasOutfit && (slot.isWorn === true || slot.isEquipped === true);

            const pathEl = document.createElementNS("http://www.w3.org/2000/svg", "path");
            pathEl.setAttribute("d", describeArc(cx, cy, rIn, rOut, startAngle, endAngle));
            pathEl.setAttribute("data-slot-index", realSlotIndex);

            let defaultFill, defaultStroke, hoverFill, hoverStroke;

            if (isEquipped) {
                defaultFill = 'rgba(16, 60, 48, 0.92)';
                defaultStroke = '#34d399';
                hoverFill = 'rgba(20, 85, 65, 0.98)';
                hoverStroke = '#6ee7b7';
                pathEl.setAttribute("class", "slice-path equipped");
            } else if (hasOutfit) {
                defaultFill = 'rgba(19, 27, 38, 0.85)';
                defaultStroke = '#6c829c';
                hoverFill = 'rgba(108, 130, 156, 0.95)';
                hoverStroke = '#cddbe8';
                pathEl.setAttribute("class", "slice-path");
            } else {
                defaultFill = 'rgba(19, 27, 38, 0.55)';
                defaultStroke = '#384553';
                hoverFill = 'rgba(56, 69, 83, 0.85)';
                hoverStroke = '#6c829c';
                pathEl.setAttribute("class", "slice-path empty");
            }

            pathEl.style.fill = defaultFill;
            pathEl.style.stroke = defaultStroke;

            pathEl.onpointerenter = () => {
                pathEl.style.fill = hoverFill;
                pathEl.style.stroke = hoverStroke;
            };

            pathEl.onpointerleave = () => {
                pathEl.style.fill = defaultFill;
                pathEl.style.stroke = defaultStroke;
            };

            const handleLeftClickTrigger = (e) => {
                if (e) {
                    e.stopPropagation();
                    e.preventDefault();
                }
                if (isEditModeActive()) {
                    saveOutfit(realSlotIndex);
                } else {
                    equipOutfit(realSlotIndex);
                }
            };

            const handleRightClickTrigger = (e) => {
                if (e) {
                    e.preventDefault();
                    e.stopPropagation();
                }
                if (!isEditModeActive()) return;
                const clickX = (e && typeof e.clientX === 'number' && e.clientX > 0) ? e.clientX : 200;
                const clickY = (e && typeof e.clientY === 'number' && e.clientY > 0) ? e.clientY : 200;
                showSlotContextMenu(clickX, clickY, realSlotIndex);
            };

            pathEl.onclick = (e) => {
                if (e && e.button === 0) {
                    handleLeftClickTrigger(e);
                }
            };

            pathEl.onmousedown = (e) => {
                if (e.button === 2 || e.which === 3) {
                    handleRightClickTrigger(e);
                }
            };

            pathEl.oncontextmenu = (e) => {
                handleRightClickTrigger(e);
            };

            svgFragment.appendChild(pathEl);

            const lx = Math.cos(midAngle) * rMid;
            const ly = Math.sin(midAngle) * rMid;

            const labelEl = document.createElement('div');
            labelEl.className = 'wheel-slot-label';
            labelEl.style.left = (cx + lx) + 'px';
            labelEl.style.top = (cy + ly) + 'px';
            labelEl.setAttribute("data-slot-index", realSlotIndex);

            if (hasOutfit) {
                labelEl.innerHTML = '<div class="slot-icon">' + svgOutfitIcon + '</div>' +
                    '<div class="slot-title">' + slot.name + '</div>' +
                    '<div class="slot-subtitle">' + slot.count + ' items</div>' +
                    (isEquipped ? '<div class="slot-equipped-badge">EQUIPPED</div>' : '');
            } else {
                labelEl.innerHTML = '<div class="slot-icon">' + svgPlusIcon + '</div>' +
                    '<div class="slot-title">Empty</div>';
            }

            labelFragment.appendChild(labelEl);
        });

        svg.appendChild(svgFragment);
        labelsContainer.appendChild(labelFragment);
    });
}

function setCategoryFilter(tag, e) {
    if (e) e.stopPropagation();
    activeCategoryFilter = tag;

    const tabs = document.querySelectorAll('.filter-tab');
    tabs.forEach(tab => {
        if (tab.getAttribute('data-tag') === tag) {
            tab.classList.add('active');
        } else {
            tab.classList.remove('active');
        }
    });

    updateWheelUI();
}

function saveOutfit(slotIndex) {
    showSaveToast("Outfit Saved!");
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
    if (now - lastEquipCallTimestamp < 350) {
        return;
    }
    lastEquipCallTimestamp = now;

    lastEquippedSlotIndex = slotIndex;
    updateWheelUI();
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
            delta: delta
        }));
    }
}

function addWheel() {
    if (typeof window.AddWheel === 'function' && currentWheelData) {
        window.AddWheel("");
    }
}

function addSlot() {
    if (typeof window.AddSlot === 'function' && currentWheelData) {
        window.AddSlot("");
    }
}

function closeWheel() {
    hideAllMenus();
    hideAllModals();
    if (typeof window.OnCloseWheel === 'function') {
        window.OnCloseWheel("");
    }
}