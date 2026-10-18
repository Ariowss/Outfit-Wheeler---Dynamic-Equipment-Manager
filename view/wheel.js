var svgPlusIcon = '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><circle cx="12" cy="12" r="9" stroke="#6c829c" stroke-width="1.5" stroke-dasharray="3 3"/><path d="M12 8V16M8 12H16" stroke="#a3b8cc" stroke-width="2" stroke-linecap="round"/></svg>';

function resolvePngIcon(slot) {
    if (!slot) return 'default.png';

    if (slot.icon && typeof slot.icon === 'string' && slot.icon.trim().length > 0) {
        let ic = slot.icon.trim();
        if (!ic.endsWith('.png')) {
            ic += '.png';
        }
        return ic;
    }

    const tag = (slot.contextTag || '').toLowerCase().trim();
    if (!tag) return 'default.png';

    if (tag.includes('sleep')) return 'action_sleep.png';
    if (tag.includes('mount') || tag.includes('horse')) return 'action_mount.png';
    if (tag.includes('swim') || tag.includes('water')) return 'action_swim.png';
    if (tag.includes('combat')) return 'action_combat.png';
    if (tag.includes('home')) return 'location_homes.png';
    if (tag.includes('inn') || tag.includes('tavern')) return 'location_inns.png';
    if (tag.includes('castle')) return 'location_castles.png';
    if (tag.includes('dungeon') || tag.includes('cave')) return 'location_dungeons.png';
    if (tag.includes('farm')) return 'location_farms.png';
    if (tag.includes('guild')) return 'location_guilds.png';
    if (tag.includes('jail')) return 'location_jails.png';
    if (tag.includes('military')) return 'location_military.png';
    if (tag.includes('store') || tag.includes('shop')) return 'location_stores.png';
    if (tag.includes('temple')) return 'location_temples.png';
    if (tag.includes('city')) return 'city.png';
    if (tag.includes('town')) return 'town.png';
    if (tag.includes('interior')) return 'interior.png';
    if (tag.includes('rain')) return 'rain.png';
    if (tag.includes('snow') || tag.includes('cold')) return 'badge_snow.png';
    if (tag.includes('night')) return 'badge_night.png';
    if (tag.includes('day')) return 'day.png';
    if (tag.includes('world') || tag.includes('adventure')) return 'world.png';

    return 'default.png';
}

function resolveSlotTorsoItem(slot) {
    if (!slot) return null;
    let items = slot.items;
    if (!items || !Array.isArray(items) || items.length === 0) {
        if (slot.itemIDs && Array.isArray(slot.itemIDs)) {
            items = slot.itemIDs;
        }
    }
    if (!items || items.length === 0) return null;

    let allInventory = [];
    if (typeof currentWheelData !== 'undefined' && currentWheelData) {
        if (Array.isArray(currentWheelData.inventory)) allInventory = allInventory.concat(currentWheelData.inventory);
        if (Array.isArray(currentWheelData.playerInventory)) allInventory = allInventory.concat(currentWheelData.playerInventory);
        if (Array.isArray(currentWheelData.cloudInventory)) allInventory = allInventory.concat(currentWheelData.cloudInventory);
        if (Array.isArray(currentWheelData.followerInventory)) allInventory = allInventory.concat(currentWheelData.followerInventory);
    }

    const resolveItem = (entry) => {
        if (!entry) return null;
        if (typeof entry === 'object') {
            const rawId = entry.rawID || entry.formID || (entry.id ? parseInt(entry.id, 16) : 0) || 0;
            const formIdStr = entry.id || ('0x' + Number(rawId).toString(16).toUpperCase());
            let thumb = entry.thumbnail || '';
            let type = entry.type || entry.category || '';
            let name = entry.name || '';
            if (!thumb || !type || !name) {
                const found = allInventory.find(x => (rawId && x.rawID === rawId) || (x.id && x.id.toUpperCase() === formIdStr.toUpperCase()));
                if (found) {
                    if (!thumb) thumb = found.thumbnail || '';
                    if (!type) type = found.type || found.category || '';
                    if (!name) name = found.name || '';
                }
            }
            return { rawID: rawId, id: formIdStr, name, type, thumbnail: thumb };
        } else {
            const formIdNum = typeof entry === 'number' ? entry : (parseInt(entry, 16) || 0);
            const formIdStr = typeof entry === 'number' ? ('0x' + entry.toString(16).toUpperCase()) : String(entry);
            const found = allInventory.find(x => (formIdNum && x.rawID === formIdNum) || (x.id && x.id.toUpperCase() === formIdStr.toUpperCase()));
            if (found) {
                return { rawID: formIdNum, id: formIdStr, name: found.name || '', type: found.type || found.category || '', thumbnail: found.thumbnail || '' };
            }
            return { rawID: formIdNum, id: formIdStr, name: 'Apparel', type: 'Armor', thumbnail: '' };
        }
    };

    const resolved = items.map(resolveItem).filter(Boolean);
    if (resolved.length === 0) return null;

    const isBox = (it) => {
        const str = ((it.name || '') + ' ' + (it.type || '')).toLowerCase();
        return str.includes('box') || str.includes('crate') || str.includes('package') || str.includes('container');
    };

    // 1. Check type: Cuirass, Torso, Robe, Body, Chest (non-box)
    let best = resolved.find(it => {
        if (isBox(it)) return false;
        const t = (it.type || '').toLowerCase();
        return t === 'cuirass' || t.includes('cuirass') || t.includes('torso') || t.includes('robe') || t.includes('body') || t.includes('chest');
    });

    // 2. Check name keywords (non-box)
    if (!best) {
        best = resolved.find(it => {
            if (isBox(it)) return false;
            const n = (it.name || '').toLowerCase();
            return n.includes('cuirass') || n.includes('armor') || n.includes('robe') || n.includes('torso') || n.includes('chest') || n.includes('body') || n.includes('tunic') || n.includes('dress') || n.includes('top');
        });
    }

    // 3. Filter out non-chest apparel (helmet, boot, glove, shield, jewelry, box)
    if (!best) {
        best = resolved.find(it => {
            if (isBox(it)) return false;
            const combined = ((it.type || '') + ' ' + (it.name || '')).toLowerCase();
            if (combined.includes('helmet') || combined.includes('hood') || combined.includes('circlet') ||
                combined.includes('gauntlet') || combined.includes('glove') || combined.includes('bracer') ||
                combined.includes('boot') || combined.includes('shoe') || combined.includes('foot') ||
                combined.includes('shield') || combined.includes('ring') || combined.includes('amulet') || combined.includes('necklace')) {
                return false;
            }
            return true;
        });
    }

    // 4. Any non-box item
    if (!best) {
        best = resolved.find(it => !isBox(it));
    }

    return best || resolved[0];
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
    const container = document.getElementById('wheel-container');
    const svg = document.getElementById('wheel-svg');
    const labelsContainer = document.getElementById('labels-container');
    const wheelIndexLbl = document.getElementById('wheel-index-label');
    const wheelNameCenter = document.getElementById('wheel-name-center');
    const wheelNameDrawer = document.getElementById('wheel-name');

    if (!svg || !labelsContainer || !container) return;

    const isEdit = isEditModeActive();
    const baseScale = (isEdit && !(typeof isClassicEditMode === 'function' && isClassicEditMode())) ? 0.65 : 1.0;
    const userScale = (currentWheelData && currentWheelData.wheelScale !== undefined) ? currentWheelData.wheelScale : 1.0;
    const offsetX = (currentWheelData && currentWheelData.wheelOffsetX !== undefined) ? currentWheelData.wheelOffsetX : 0.0;
    const offsetY = (currentWheelData && currentWheelData.wheelOffsetY !== undefined) ? currentWheelData.wheelOffsetY : 0.0;

    container.style.transform = `translate3d(${offsetX}px, ${offsetY}px, 0) scale(${baseScale * userScale})`;

    if (wheelIndexLbl) {
        wheelIndexLbl.innerText = `WHEEL ${(currentWheelData.wheelIndex || 0) + 1} OF ${currentWheelData.totalWheels || 1}`;
    }

    const defaultWheelTitle = currentWheelData.wheelName || ('Wheel ' + ((currentWheelData.wheelIndex || 0) + 1));
    if (wheelNameDrawer) {
        wheelNameDrawer.innerText = defaultWheelTitle;
    }
    if (wheelNameCenter) {
        wheelNameCenter.innerText = defaultWheelTitle;
    }

    svg.innerHTML = '';
    labelsContainer.innerHTML = '';

    const rawSlots = currentWheelData.slots || [];
    let displaySlots = [];

    function matchesContext(s, filter) {
        if (!s) return false;
        if (!filter || filter === 'ALL' || filter.toUpperCase() === 'ALL') return true;
        const tag = (s.contextTag || '').toLowerCase().trim();
        if (!tag) return false;
        const f = filter.toLowerCase().trim();
        if (tag.includes(f)) return true;
        if (f === 'city' && (tag.includes('town') || tag.includes('village'))) return true;
        if (f === 'home' && (tag.includes('house') || tag.includes('playerhome') || tag.includes('inn') || tag.includes('tavern') || tag.includes('bed') || tag.includes('sleep') || tag.includes('rest'))) return true;
        if (f === 'combat' && (tag.includes('fight') || tag.includes('war') || tag.includes('military') || tag.includes('dungeon'))) return true;
        if (f === 'swim' && (tag.includes('water') || tag.includes('inwater') || tag.includes('in_water'))) return true;
        return false;
    }

    if (activeWheelContextFilter && activeWheelContextFilter !== 'ALL') {
        rawSlots.forEach((s, idx) => {
            const realIdx = (s && s.index !== undefined) ? s.index : idx;
            const hasOutfit = s && (s.hasOutfit === true || (s.items && s.items.length > 0) || (s.count !== undefined && s.count > 0));
            if (matchesContext(s, activeWheelContextFilter)) {
                if (isEdit || hasOutfit) {
                    displaySlots.push({ ...s, realIndex: realIdx });
                }
            } else if (isEdit && (!s.contextTag || s.contextTag.trim().length === 0)) {
                displaySlots.push({ ...s, realIndex: realIdx });
            }
        });
    } else {
        rawSlots.forEach((s, idx) => {
            const realIdx = (s && s.index !== undefined) ? s.index : idx;
            const hasOutfit = s && (s.hasOutfit === true || (s.items && s.items.length > 0) || (s.count !== undefined && s.count > 0));
            if (isEdit || hasOutfit) {
                displaySlots.push({ ...s, realIndex: realIdx });
            }
        });
    }

    const total = displaySlots.length;
    if (total === 0) {
        const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
        circle.setAttribute("cx", "290");
        circle.setAttribute("cy", "290");
        circle.setAttribute("r", "180");
        circle.setAttribute("class", "slice-path empty");
        circle.style.fill = 'rgba(13, 18, 26, 0.5)';
        circle.style.stroke = 'rgba(45, 58, 75, 0.4)';
        circle.style.strokeDasharray = '4 4';
        svg.appendChild(circle);

        const emptyMsg = document.createElement('div');
        emptyMsg.className = 'wheel-slot-label';
        emptyMsg.style.left = '290px';
        emptyMsg.style.top = '290px';
        const isFollower = currentWheelData && currentWheelData.activeTargetActorId && currentWheelData.activeTargetActorId.toUpperCase() !== '0X00000007' && currentWheelData.activeTargetActorId.toUpperCase() !== '0X7';
        if (isFollower && !isEdit) {
            emptyMsg.innerHTML = '<div class="slot-title" style="color:#94a3b8;">Follower has no outfits set</div><div style="font-size:10px; color:#64748b; margin-top:4px;">Open Edit Mode to configure</div>';
        } else {
            emptyMsg.innerHTML = '<div class="slot-title" style="color:#64748b;">No Outfits in Filter</div>';
        }
        labelsContainer.appendChild(emptyMsg);
        return;
    }

    const cx = 290;
    const cy = 290;
    const rIn = 110;
    const rOut = 270;
    const rMid = (rIn + rOut) / 2;

    const sliceAngle = (2 * Math.PI) / total;
    const gapAngle = total > 1 ? 0.025 : 0;

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

        const slotCustomColor = (slot.color && typeof slot.color === 'string' && slot.color.trim().length > 0) ? slot.color.trim() : null;
        const wheelCustomColor = (currentWheelData && currentWheelData.wheelColor && typeof currentWheelData.wheelColor === 'string' && currentWheelData.wheelColor.trim().length > 0) ? currentWheelData.wheelColor.trim() : null;
        const effectiveCustomColor = slotCustomColor || wheelCustomColor || null;

        function hexToRgbaStr(hex, alpha) {
            if (!hex || typeof hex !== 'string') return `rgba(255, 255, 255, ${alpha})`;
            let c = hex.replace('#', '');
            if (c.length === 3) c = c.split('').map(x => x + x).join('');
            if (c.length !== 6) return `rgba(255, 255, 255, ${alpha})`;
            const num = parseInt(c, 16);
            const r = (num >> 16) & 255;
            const g = (num >> 8) & 255;
            const b = num & 255;
            return `rgba(${r}, ${g}, ${b}, ${alpha})`;
        }

        if (effectiveCustomColor) {
            if (isAddon) {
                defaultFill = isEquipped ? hexToRgbaStr(effectiveCustomColor, 0.40) : hexToRgbaStr(effectiveCustomColor, 0.15);
                defaultStroke = effectiveCustomColor;
                hoverFill = hexToRgbaStr(effectiveCustomColor, 0.60);
                hoverStroke = '#ffffff';
                pathEl.setAttribute("class", isEquipped ? "slice-path equipped" : "slice-path");
            } else if (isEquipped) {
                defaultFill = hexToRgbaStr(effectiveCustomColor, 0.35);
                defaultStroke = effectiveCustomColor;
                hoverFill = hexToRgbaStr(effectiveCustomColor, 0.55);
                hoverStroke = '#ffffff';
                pathEl.setAttribute("class", "slice-path equipped");
            } else if (hasOutfit) {
                defaultFill = hexToRgbaStr(effectiveCustomColor, 0.18);
                defaultStroke = hexToRgbaStr(effectiveCustomColor, 0.70);
                hoverFill = hexToRgbaStr(effectiveCustomColor, 0.38);
                hoverStroke = effectiveCustomColor;
                pathEl.setAttribute("class", "slice-path");
            } else {
                defaultFill = 'rgba(13, 18, 26, 0.55)';
                defaultStroke = hexToRgbaStr(effectiveCustomColor, 0.35);
                hoverFill = 'rgba(255, 255, 255, 0.08)';
                hoverStroke = effectiveCustomColor;
                pathEl.setAttribute("class", "slice-path empty");
            }
        } else if (isAddon) {
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
                    if (typeof isClassicEditMode === 'function' && isClassicEditMode()) {
                        selectedSlotIndex = realSlotIndex;
                        if (typeof showSlotContextMenu === 'function') {
                            showSlotContextMenu(e ? e.clientX : 0, e ? e.clientY : 0, realSlotIndex);
                        }
                    } else {
                        selectedSlotIndex = realSlotIndex;
                        renderSlotsRail();
                        renderDrawerDetails();
                        const slotObj = (currentWheelData.slots || []).find(s => (s.index !== undefined ? s.index : 0) === realSlotIndex) || (currentWheelData.slots || [])[realSlotIndex];
                        if (slotObj && slotObj.hasOutfit && slotObj.items && slotObj.items.length > 0) {
                            equipOutfit(realSlotIndex, true);
                        }
                    }
                } else {
                    const keepOpen = (e && e.shiftKey === true) || (typeof isShiftKeyDown !== 'undefined' && isShiftKeyDown) || (currentWheelData && currentWheelData.autoCloseOnEquip === false);
                    equipOutfit(realSlotIndex, keepOpen);
                }
            }
        };

        pathEl.oncontextmenu = (e) => {
            if (e) {
                e.preventDefault();
                e.stopPropagation();
            }
            if (isEditModeActive()) {
                selectedSlotIndex = realSlotIndex;
                if (typeof showSlotContextMenu === 'function') {
                    showSlotContextMenu(e ? e.clientX : 0, e ? e.clientY : 0, realSlotIndex);
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

        labelEl.onclick = (e) => {
            if (e && e.button === 0) {
                if (isEditModeActive()) {
                    if (typeof isClassicEditMode === 'function' && isClassicEditMode()) {
                        selectedSlotIndex = realSlotIndex;
                        if (typeof showSlotContextMenu === 'function') {
                            showSlotContextMenu(e ? e.clientX : 0, e ? e.clientY : 0, realSlotIndex);
                        }
                    } else {
                        selectedSlotIndex = realSlotIndex;
                        renderSlotsRail();
                        renderDrawerDetails();
                        const slotObj = (currentWheelData.slots || []).find(s => (s.index !== undefined ? s.index : 0) === realSlotIndex) || (currentWheelData.slots || [])[realSlotIndex];
                        if (slotObj && slotObj.hasOutfit && slotObj.items && slotObj.items.length > 0) {
                            equipOutfit(realSlotIndex, true);
                        }
                    }
                } else {
                    const keepOpen = (e && e.shiftKey === true) || (typeof isShiftKeyDown !== 'undefined' && isShiftKeyDown) || (currentWheelData && currentWheelData.autoCloseOnEquip === false);
                    equipOutfit(realSlotIndex, keepOpen);
                }
            }
        };

        labelEl.oncontextmenu = (e) => {
            if (e) {
                e.preventDefault();
                e.stopPropagation();
            }
            if (isEditModeActive()) {
                selectedSlotIndex = realSlotIndex;
                if (typeof showSlotContextMenu === 'function') {
                    showSlotContextMenu(e ? e.clientX : 0, e ? e.clientY : 0, realSlotIndex);
                }
            }
        };

        if (hasOutfit) {
            const iconName = resolvePngIcon(slot);
            const previewsEnabled = (typeof isItemPreviewsEnabled === 'function') ? isItemPreviewsEnabled() : true;
            const torsoItem = previewsEnabled ? resolveSlotTorsoItem(slot) : null;

            let iconHtml = '';
            if (torsoItem && torsoItem.rawID && previewsEnabled) {
                if (typeof queueThumbnailRequest === 'function' && (!torsoItem.thumbnail || torsoItem.thumbnail.length === 0)) {
                    queueThumbnailRequest(torsoItem.rawID, false);
                }
                const hasThumb = (torsoItem.thumbnail && torsoItem.thumbnail.length > 0);
                if (hasThumb) {
                    iconHtml = '<div class="slot-icon">' +
                        '<img class="slot-icon-img wheel-slot-thumb-img" data-raw-id="' + torsoItem.rawID + '" src="' + torsoItem.thumbnail + '" alt="" onerror="this.style.display=\'none\'; if(this.nextElementSibling) this.nextElementSibling.style.display=\'block\';">' +
                        '<img class="slot-icon-img fallback-icon" src="icons/' + iconName + '" alt="" style="display:none;">' +
                        '</div>';
                } else {
                    iconHtml = '<div class="slot-icon">' +
                        '<img class="slot-icon-img fallback-icon" src="icons/' + iconName + '" alt="">' +
                        '<img class="slot-icon-img wheel-slot-thumb-img" data-raw-id="' + torsoItem.rawID + '" src="" alt="" style="display:none;" onerror="this.style.display=\'none\'; if(this.previousElementSibling) this.previousElementSibling.style.display=\'block\';">' +
                        '</div>';
                }
            } else {
                iconHtml = '<div class="slot-icon"><img class="slot-icon-img" src="icons/' + iconName + '" alt=""></div>';
            }

            const displayName = slot.name || ('Slot ' + (realSlotIndex + 1));
            const itemCount = (slot.items && slot.items.length > 0) ? slot.items.length : (slot.count !== undefined ? slot.count : 0);
            const colorDot = slotCustomColor ? `<span style="display:inline-block; width:7px; height:7px; border-radius:50%; background:${slotCustomColor}; margin-right:4px; box-shadow:0 0 6px ${slotCustomColor}; vertical-align:middle;"></span>` : '';

            labelEl.innerHTML = iconHtml +
                '<div class="slot-title">' + colorDot + displayName + '</div>' +
                '<div class="slot-subtitle">' + itemCount + ' items</div>' +
                (isEquipped ? '<div class="slot-equipped-badge">EQUIPPED</div>' : '');
        } else {
            labelEl.innerHTML = '<div class="slot-icon">' + svgPlusIcon + '</div>' +
                '<div class="slot-title">Empty</div>';
        }

        labelFragment.appendChild(labelEl);
    });

    svg.appendChild(svgFragment);
    labelsContainer.appendChild(labelFragment);
}