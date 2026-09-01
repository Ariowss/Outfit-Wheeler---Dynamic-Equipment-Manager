var updateWheelRafId = null;

var svgOutfitIcon = '<svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20.38 3.46L16 2a4 4 0 01-8 0L3.62 3.46a2 2 0 00-1.34 2.23l.58 3.47a1 1 0 00.99.84H6v10c0 1.1.9 2 2 2h8a2 2 0 002-2V10h2.15a1 1 0 00.99-.84l.58-3.47a2 2 0 00-1.34-2.23z"/></svg>';
var svgPlusIcon = '<svg width="26" height="26" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>';

function escapeHtml(str) {
    if (!str) return '';
    return String(str)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

function applyTransform() {
    const container = document.getElementById('wheel-container');
    if (!container || !currentWheelData) return;
    const isEdit = isEditModeActive();
    const baseScale = isEdit ? 0.65 : 1.0;
    const userScale = (currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
    const autoScale = parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--auto-ui-scale')) || 1.0;
    const offsetX = (currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0;
    const offsetY = (currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0;

    container.style.transform = 'translate3d(' + Math.round(offsetX) + 'px, ' + Math.round(offsetY) + 'px, 0) scale(' + (baseScale * userScale * autoScale) + ')';
}

function describeArc(x, y, innerRadius, outerRadius, startAngle, endAngle) {
    if (endAngle - startAngle >= (2 * Math.PI) - 0.01) {
        const mid = startAngle + Math.PI;
        const x1_out = x + outerRadius * Math.cos(startAngle);
        const y1_out = y + outerRadius * Math.sin(startAngle);
        const xm_out = x + outerRadius * Math.cos(mid);
        const ym_out = y + outerRadius * Math.sin(mid);
        const x1_in = x + innerRadius * Math.cos(startAngle);
        const y1_in = y + innerRadius * Math.sin(startAngle);
        const xm_in = x + innerRadius * Math.cos(mid);
        const ym_in = y + innerRadius * Math.sin(mid);

        return [
            "M " + x1_out + " " + y1_out,
            "A " + outerRadius + " " + outerRadius + " 0 1 1 " + xm_out + " " + ym_out,
            "A " + outerRadius + " " + outerRadius + " 0 1 1 " + x1_out + " " + y1_out,
            "M " + x1_in + " " + y1_in,
            "A " + innerRadius + " " + innerRadius + " 0 1 0 " + xm_in + " " + ym_in,
            "A " + innerRadius + " " + innerRadius + " 0 1 0 " + x1_in + " " + y1_in,
            "Z"
        ].join(" ");
    }

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

function resolveIconFileName(slot) {
    if (!slot) return 'world';

    if (slot.icon && typeof slot.icon === 'string' && slot.icon.length > 0) {
        return slot.icon.replace(/\.[^/.]+$/, "");
    }

    if (slot.contextTag && typeof slot.contextTag === 'string' && slot.contextTag.length > 0) {
        const tag = slot.contextTag.toLowerCase();
        if (tag.includes('combat') || tag.includes('fight')) return 'action_combat';
        if (tag.includes('mount') || tag.includes('horse')) return 'action_mount';
        if (tag.includes('sleep') || tag.includes('bed')) return 'action_sleep';
        if (tag.includes('swim') || tag.includes('water')) return 'action_swim';

        if (tag.includes('castle') || tag.includes('palace')) return 'location_castles';
        if (tag.includes('dung') || tag.includes('tomb') || tag.includes('ruin')) return 'location_dungeons';
        if (tag.includes('farm') || tag.includes('mill')) return 'location_farms';
        if (tag.includes('guild') || tag.includes('thief') || tag.includes('brotherhood')) return 'location_guilds';
        if (tag.includes('home') || tag.includes('house')) return 'location_homes';
        if (tag.includes('inn') || tag.includes('tavern')) return 'location_inns';
        if (tag.includes('jail') || tag.includes('prison')) return 'location_jails';
        if (tag.includes('military') || tag.includes('camp') || tag.includes('war')) return 'location_military';
        if (tag.includes('store') || tag.includes('shop') || tag.includes('market')) return 'location_stores';
        if (tag.includes('temple') || tag.includes('shrine') || tag.includes('talos')) return 'location_temples';

        if (tag.includes('interior')) return 'interior';
        if (tag.includes('city')) return 'city';
        if (tag.includes('town') || tag.includes('village')) return 'town';
        if (tag.includes('world') || tag.includes('adventure') || tag.includes('nature')) return 'world';
    }

    return 'world';
}

function renderSlotIconHtml(slot) {
    if (!slot || !slot.hasOutfit) {
        return '<span class="slot-fallback-icon">' + svgPlusIcon + '</span>';
    }

    const baseName = resolveIconFileName(slot);
    const mainIconSrc = 'icons/' + baseName + '.png';

    let subBadges = [];
    const tag = (slot.contextTag && typeof slot.contextTag === 'string') ? slot.contextTag.toLowerCase() : '';

    if (tag.includes('night')) {
        subBadges.push('<img class="sub-badge-img" src="icons/badge_night.png" alt="" />');
    } else if (tag.includes('day')) {
        subBadges.push('<img class="sub-badge-img" src="icons/day.png" alt="" />');
    }

    if (tag.includes('rain')) {
        subBadges.push('<img class="sub-badge-img" src="icons/rain.png" alt="" />');
    } else if (tag.includes('snow')) {
        subBadges.push('<img class="sub-badge-img" src="icons/badge_snow.png" alt="" />');
    }

    if (baseName !== 'interior' && tag.includes('interior') && subBadges.length < 2) {
        subBadges.push('<img class="sub-badge-img" src="icons/interior.png" alt="" />');
    }

    let badgeRow = subBadges.length > 0 ? '<div class="slot-badges-row">' + subBadges.join('') + '</div>' : '';

    return '<div class="slot-icon-wrapper">' +
        '<img class="slot-icon-img" src="' + mainIconSrc + '" onerror="this.style.display=\'none\'; if(this.nextElementSibling) this.nextElementSibling.style.display=\'inline-flex\';" />' +
        '<span class="slot-fallback-icon" style="display:none;">' + svgOutfitIcon + '</span>' +
        badgeRow +
        '</div>';
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

        applyTransform();

        if (wheelIndexLbl) {
            wheelIndexLbl.innerText = `WHEEL ${(currentWheelData.wheelIndex || 0) + 1} OF ${currentWheelData.totalWheels || 1}`;
        }

        const defaultWheelTitle = (currentWheelData && currentWheelData.wheelName) ? currentWheelData.wheelName : ('Wheel ' + (((currentWheelData && currentWheelData.wheelIndex) || 0) + 1));
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

        const isEdit = isEditModeActive();
        const rawSlots = (currentWheelData && currentWheelData.slots) ? currentWheelData.slots : [];
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
                if (s && s.hasOutfit && s.contextTag && typeof s.contextTag === 'string') {
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
        const gapAngle = total === 1 ? 0 : Math.min(0.02, sliceAngle * 0.08);

        const svgFragment = document.createDocumentFragment();
        const labelFragment = document.createDocumentFragment();

        displaySlots.forEach((slot, index) => {
            let startAngle, endAngle;
            if (total === 1) {
                startAngle = -Math.PI / 2;
                endAngle = startAngle + (2 * Math.PI);
            } else {
                startAngle = (index * sliceAngle) - (Math.PI / 2) + (gapAngle / 2);
                endAngle = ((index + 1) * sliceAngle) - (Math.PI / 2) - (gapAngle / 2);
            }

            const midAngle = total === 1 ? (-Math.PI / 2) : ((startAngle + endAngle) / 2);
            const hasOutfit = slot && slot.hasOutfit;
            const realSlotIndex = slot.realIndex !== undefined ? slot.realIndex : index;
            const isEquipped = hasOutfit && (slot.isWorn === true || slot.isEquipped === true);
            const isAddon = slot.isAddon === true;
            const isSelected = isEdit && (realSlotIndex === selectedSlotIndex);

            const pathEl = document.createElementNS("http://www.w3.org/2000/svg", "path");
            pathEl.setAttribute("d", describeArc(cx, cy, rIn, rOut, startAngle, endAngle));
            pathEl.setAttribute("data-slot-index", realSlotIndex);

            let defaultFill, defaultStroke, hoverFill, hoverStroke;

            if (isAddon) {
                defaultFill = isEquipped ? 'rgba(168, 85, 247, 0.35)' : 'rgba(168, 85, 247, 0.12)';
                defaultStroke = isSelected ? '#ffffff' : (isEquipped ? '#e9d5ff' : '#c084fc');
                hoverFill = 'rgba(168, 85, 247, 0.55)';
                hoverStroke = '#ffffff';
                pathEl.setAttribute("class", isEquipped ? "slice-path addon-equipped" : "slice-path");
            } else if (isEquipped) {
                defaultFill = 'rgba(16, 185, 129, 0.25)';
                defaultStroke = isSelected ? '#ffffff' : '#10b981';
                hoverFill = 'rgba(16, 185, 129, 0.45)';
                hoverStroke = '#34d399';
                pathEl.setAttribute("class", "slice-path equipped");
            } else if (hasOutfit) {
                defaultFill = 'rgba(13, 18, 26, 0.88)';
                defaultStroke = isSelected ? '#52a5f5' : 'rgba(45, 58, 75, 0.75)';
                hoverFill = 'rgba(255, 255, 255, 0.18)';
                hoverStroke = '#f8fafc';
                pathEl.setAttribute("class", "slice-path");
            } else {
                defaultFill = 'rgba(13, 18, 26, 0.55)';
                defaultStroke = isSelected ? '#52a5f5' : 'rgba(45, 58, 75, 0.45)';
                hoverFill = 'rgba(255, 255, 255, 0.08)';
                hoverStroke = '#94a3b8';
                pathEl.setAttribute("class", "slice-path empty");
            }

            if (isSelected) {
                pathEl.style.strokeWidth = "3px";
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
                        updateWheelUI();
                    } else {
                        const keepOpen = (e.shiftKey === true || currentWheelData.autoCloseOnEquip === false);
                        equipOutfit(realSlotIndex, keepOpen);
                    }
                }
            };

            svgFragment.appendChild(pathEl);

            const lx = Math.cos(midAngle) * rMid;
            const ly = Math.sin(midAngle) * rMid;

            const labelEl = document.createElement('div');
            labelEl.className = 'wheel-slot-label';
            labelEl.style.left = Math.round(cx + lx) + 'px';
            labelEl.style.top = Math.round(cy + ly) + 'px';
            labelEl.setAttribute("data-slot-index", realSlotIndex);

            let badgeHtml = '';
            if (isAddon && isEquipped) {
                badgeHtml = '<div class="slot-equipped-badge badge-addon-equipped">ADDON • ON</div>';
            } else if (isAddon) {
                badgeHtml = '<div class="slot-equipped-badge badge-addon-off">ADDON (OFF)</div>';
            } else if (isEquipped) {
                badgeHtml = '<div class="slot-equipped-badge">EQUIPPED</div>';
            }

            if (hasOutfit) {
                labelEl.innerHTML = '<div class="slot-icon">' + renderSlotIconHtml(slot) + '</div>' +
                    '<div class="slot-title">' + escapeHtml(slot.name || 'Outfit') + '</div>' +
                    '<div class="slot-subtitle">' + ((slot.count || 0) + ' items') + '</div>' +
                    badgeHtml;
            } else {
                labelEl.innerHTML = '<div class="slot-icon"><span class="slot-fallback-icon">' + svgPlusIcon + '</span></div>' +
                    '<div class="slot-title">Empty</div>';
            }

            labelFragment.appendChild(labelEl);
        });

        svg.appendChild(svgFragment);
        labelsContainer.appendChild(labelFragment);
    });
}