var updateWheelRafId = null;

var svgOutfitIcon = '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20.38 3.46L16 2a4 4 0 01-8 0L3.62 3.46a2 2 0 00-1.34 2.23l.58 3.47a1 1 0 00.99.84H6v10c0 1.1.9 2 2 2h8a2 2 0 002-2V10h2.15a1 1 0 00.99-.84l.58-3.47a2 2 0 00-1.34-2.23z"/></svg>';
var svgPlusIcon = '<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>';

function escapeHtml(str) {
    if (!str) return '';
    return String(str)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

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
    if (updateWheelRafId) {
        window.cancelAnimationFrame(updateWheelRafId);
    }
    updateWheelRafId = window.requestAnimationFrame(() => {
        updateWheelRafId = null;
        const container = document.getElementById('wheel-container');
        const svg = document.getElementById('wheel-svg');
        const labelsContainer = document.getElementById('labels-container');
        const wheelIndexLbl = document.getElementById('wheel-index-label');
        const wheelNameCenter = document.getElementById('wheel-name-center');
        const wheelNameDrawer = document.getElementById('wheel-name');

        if (!svg || !labelsContainer || !container) return;

        const isEdit = isEditModeActive();
        const baseScale = isEdit ? 0.55 : 1.0;
        const userScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
        const offsetX = isEdit ? 0.0 : ((currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0);
        const offsetY = isEdit ? 0.0 : ((currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0);

        container.style.transform = 'translateX(' + offsetX + 'px) translateY(' + offsetY + 'px) scale(' + (baseScale * userScale) + ')';

        if (wheelIndexLbl) {
            wheelIndexLbl.innerText = `WHEEL ${(currentWheelData.wheelIndex || 0) + 1} OF ${currentWheelData.totalWheels || 1}`;
        }

        const defaultWheelTitle = currentWheelData.wheelName || ('Wheel ' + ((currentWheelData.wheelIndex || 0) + 1));
        if (wheelNameDrawer) {
            wheelNameDrawer.innerText = defaultWheelTitle;
        }
        if (wheelNameCenter) {
            if (activeWheelContextFilter !== 'ALL') {
                wheelNameCenter.innerText = activeWheelContextFilter + ' View';
            } else {
                wheelNameCenter.innerText = defaultWheelTitle;
            }
        }

        svg.innerHTML = '';
        labelsContainer.innerHTML = '';

        const rawSlots = currentWheelData.slots || [];
        let displaySlots = [];

        if (activeWheelContextFilter === 'ALL') {
            rawSlots.forEach((s, idx) => {
                const realIdx = s.index !== undefined ? s.index : idx;
                const hasOutfit = s && s.hasOutfit && (s.count === undefined || s.count > 0 || (s.items && s.items.length > 0));
                if (isEdit || hasOutfit) {
                    displaySlots.push({ ...s, realIndex: realIdx });
                }
            });
        } else {
            const filterTag = activeWheelContextFilter.toLowerCase();
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
        const rOut = 265;
        const rMid = (rIn + rOut) / 2;

        const sliceAngle = (2 * Math.PI) / total;
        const gapAngle = Math.min(0.02, sliceAngle * 0.08);

        const svgFragment = document.createDocumentFragment();
        const labelFragment = document.createDocumentFragment();

        displaySlots.forEach((slot, index) => {
            let startAngle, endAngle;
            if (total === 1) {
                startAngle = -Math.PI / 2;
                endAngle = startAngle + (2 * Math.PI) - 0.001;
            } else {
                startAngle = (index * sliceAngle) - (Math.PI / 2) + (gapAngle / 2);
                endAngle = ((index + 1) * sliceAngle) - (Math.PI / 2) - (gapAngle / 2);
            }

            const midAngle = (startAngle + endAngle) / 2;
            const hasOutfit = slot.hasOutfit;
            const realSlotIndex = slot.realIndex;
            const isEquipped = hasOutfit && (slot.isWorn === true || slot.isEquipped === true);
            const isAddon = slot.isAddon === true;

            const pathEl = document.createElementNS("http://www.w3.org/2000/svg", "path");
            pathEl.setAttribute("d", describeArc(cx, cy, rIn, rOut, startAngle, endAngle));
            pathEl.setAttribute("data-slot-index", realSlotIndex);

            let defaultFill, defaultStroke, hoverFill, hoverStroke;

            if (isAddon) {
                defaultFill = isEquipped ? 'rgba(168, 85, 247, 0.35)' : 'rgba(168, 85, 247, 0.12)';
                defaultStroke = '#c084fc';
                hoverFill = 'rgba(168, 85, 247, 0.55)';
                hoverStroke = '#e9d5ff';
                pathEl.setAttribute("class", isEquipped ? "slice-path equipped" : "slice-path");
            } else if (isEquipped) {
                defaultFill = 'rgba(16, 185, 129, 0.25)';
                defaultStroke = '#10b981';
                hoverFill = 'rgba(16, 185, 129, 0.45)';
                hoverStroke = '#34d399';
                pathEl.setAttribute("class", "slice-path equipped");
            } else if (hasOutfit) {
                defaultFill = 'rgba(13, 18, 26, 0.88)';
                defaultStroke = 'rgba(45, 58, 75, 0.55)';
                hoverFill = 'rgba(255, 255, 255, 0.18)';
                hoverStroke = '#f8fafc';
                pathEl.setAttribute("class", "slice-path");
            } else {
                defaultFill = 'rgba(13, 18, 26, 0.55)';
                defaultStroke = 'rgba(45, 58, 75, 0.35)';
                hoverFill = 'rgba(255, 255, 255, 0.08)';
                hoverStroke = '#94a3b8';
                pathEl.setAttribute("class", "slice-path empty");
            }

            if (gamepadHoverSlotIndex === index) {
                pathEl.classList.add('gamepad-hover');
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

            pathEl.onclick = (e) => {
                if (e && e.button === 0) {
                    if (isEditModeActive()) {
                        selectedSlotIndex = realSlotIndex;
                        renderSlotsRail();
                        renderDrawerDetails();
                    } else {
                        equipOutfit(realSlotIndex);
                    }
                }
            };

            svgFragment.appendChild(pathEl);

            const lx = Math.cos(midAngle) * rMid;
            const ly = Math.sin(midAngle) * rMid;

            const labelEl = document.createElement('div');
            labelEl.className = 'wheel-slot-label';
            labelEl.style.left = (cx + lx) + 'px';
            labelEl.style.top = (cy + ly) + 'px';
            labelEl.setAttribute("data-slot-index", realSlotIndex);

            let badgeHtml = '';
            if (isAddon) {
                badgeHtml = '<div class="slot-equipped-badge" style="color:#c084fc; background:rgba(168,85,247,0.25);">ADDON</div>';
            } else if (isEquipped) {
                badgeHtml = '<div class="slot-equipped-badge">EQUIPPED</div>';
            }

            if (hasOutfit) {
                labelEl.innerHTML = `<div class="slot-icon">${svgOutfitIcon}</div>` +
                    `<div class="slot-title">${escapeHtml(slot.name)}</div>` +
                    `<div class="slot-subtitle">${(slot.count || 0) + ' items'}</div>` +
                    badgeHtml;
            } else {
                labelEl.innerHTML = `<div class="slot-icon">${svgPlusIcon}</div>` +
                    '<div class="slot-title">Empty</div>';
            }

            labelFragment.appendChild(labelEl);
        });

        svg.appendChild(svgFragment);
        labelsContainer.appendChild(labelFragment);
    });
}