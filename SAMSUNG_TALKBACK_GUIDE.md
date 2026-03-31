# TalkBack Accessibility Guide for SmartThings MapView

**Godot Engine v4.6.1 — TalkBack AAR v12**
**Date:** March 31, 2026

---

## What v12 Delivers (Engine-Level)

The `godot-lib-4.6.1-talkback-v12.aar` includes the following accessibility capabilities baked into the engine:

| Capability | Description |
|---|---|
| **TalkBack touch exploration** | Finger hover over any 3D object announces its name and draws a focus rectangle |
| **Swipe navigation containment** | Left/right swipe stays inside the Godot MapView — does not escape to Android native views (bottom bar, top bar) |
| **AABB screen projection** | 3D objects get accessibility bounds that match their actual screen footprint — room floors get large rectangles, device icons get correctly-sized boxes |
| **Duplicate-position deduplication** | Multiple nodes stacked at the same screen position (e.g., IconBase, ModelBase, DeviceObjectInfo, InputManager) are collapsed to a single focusable item |
| **Collection semantics** | Godot's virtual node tree is presented to TalkBack as an ordered list — TalkBack announces "Item X" and navigates sequentially |
| **CollectionInfo NPE fix** | Prevents TalkBack crash on `CollectionInfo.getItemCount()` for collection-type nodes |
| **Screen reader detection** | `accessibility_screen_reader_active()` returns `true` when TalkBack is enabled (cached, low overhead) |
| **Parent node focusability** | Room nodes (CSGBox3D, MeshInstance3D) are focusable even if they contain child nodes — not restricted to leaf-only |

---

## What the SmartThings GDScript Team Needs To Do

The engine provides the accessibility infrastructure, but **the GDScript scene must configure semantic information** for TalkBack to announce meaningful content and navigate in the correct order.

### 1. Set `accessibility_name` on Every Interactive Node

Without this, TalkBack reads the raw Godot node name (e.g., `"CSGBox146927e7-f2c4-40dc-b978-643be768e020"`).

```gdscript
# Room nodes
$LivingRoom.accessibility_name = "Living Room"
$GuestRoom.accessibility_name = "Guest Room"
$MasterBedroom.accessibility_name = "Master Bedroom"
$Kitchen.accessibility_name = "Kitchen"

# Device nodes
$SmartLight.accessibility_name = "Smart Light, Living Room"
$Thermostat.accessibility_name = "Thermostat, Master Bedroom"
$SmartTV.accessibility_name = "Smart TV, Guest Room"
$AirConditioner.accessibility_name = "Air Conditioner, Kitchen"
```

> **Tip:** Include the room name in the device description (e.g., "Smart Light, Living Room") so TalkBack users know which room the device belongs to.

### 2. Organize the Scene Tree: Rooms First, Then Devices

TalkBack swipe navigation follows the **Godot scene tree order** (depth-first traversal). To achieve "rooms first, then devices" navigation:

```
MapRoot (Node3D)
  |
  |-- RoomsGroup (Node3D)          <-- plain Node3D container, not focusable
  |     |-- LivingRoom (CSGBox3D)  <-- accessibility_name = "Living Room"
  |     |-- GuestRoom (CSGBox3D)   <-- accessibility_name = "Guest Room"
  |     |-- MasterRoom (CSGBox3D)  <-- accessibility_name = "Master Bedroom"
  |     |-- Kitchen (CSGBox3D)     <-- accessibility_name = "Kitchen"
  |
  |-- DevicesGroup (Node3D)        <-- plain Node3D container, not focusable
        |-- Light1 (MeshInstance3D)    <-- accessibility_name = "Smart Light, Living Room"
        |-- Thermostat1 (MeshInstance3D) <-- accessibility_name = "Thermostat, Master Bedroom"
        |-- TV1 (MeshInstance3D)       <-- accessibility_name = "Smart TV, Guest Room"
        |-- AC1 (MeshInstance3D)       <-- accessibility_name = "Air Conditioner, Kitchen"
```

**Result:** Swipe right goes: Living Room -> Guest Room -> Master Bedroom -> Kitchen -> Smart Light -> Thermostat -> Smart TV -> Air Conditioner.

**Key rule:** Rooms must NOT contain devices as children. Use two separate groups — `RoomsGroup` and `DevicesGroup` — both under the map root. The grouping `Node3D` containers are transparent to accessibility (they get `ROLE_CONTAINER` and are skipped by TalkBack).

### 3. Room Nodes Must Be GeometryInstance3D

The engine only makes `GeometryInstance3D` subclasses (CSGBox3D, MeshInstance3D, CSGCombiner3D, etc.) focusable. Plain `Node3D` containers are NOT focusable — they are treated as structural.

If rooms are currently represented by plain `Node3D` with child meshes:

```
# BAD — Node3D is not focusable, TalkBack skips it
LivingRoom (Node3D)
  |-- FloorMesh (MeshInstance3D)
  |-- WallMesh (MeshInstance3D)
```

Change to either:

```
# GOOD — CSGBox3D for the floor, TalkBack can focus it
LivingRoom (CSGBox3D)        <-- accessibility_name = "Living Room"
  |-- WallMesh (MeshInstance3D)
```

Or use a `MeshInstance3D` with the room floor mesh as the room's root node.

### 4. Use `accessibility_description` for Extra Context (Optional)

```gdscript
$LivingRoom.accessibility_name = "Living Room"
$LivingRoom.accessibility_description = "3 devices, 1 active"

$SmartLight.accessibility_name = "Smart Light"
$SmartLight.accessibility_description = "Living Room, currently on"
```

TalkBack reads the name first, then the description after a brief pause.

---

## Swipe Navigation Behavior Summary

Once the scene tree is configured as described above:

| Gesture | Behavior |
|---|---|
| **Right swipe** | Moves to next item: rooms first (all rooms), then devices (all devices) |
| **Left swipe** | Moves to previous item: reverse order |
| **Right swipe past last device** | Stays inside Godot view (scroll containment) |
| **Left swipe past first room** | Exits Godot view to Android native UI (top bar) |
| **Touch/hover on 3D object** | Focuses and announces that specific object |
| **Double-tap** | Activates the currently focused item (sends click action) |

---

## Troubleshooting

### TalkBack reads "CSGBox146927e7..." instead of "Living Room"
`accessibility_name` is not set on the node. Set it in GDScript or in the Godot editor's inspector panel.

### A room is not focusable by TalkBack
The room node is either a plain `Node3D` (not a geometry type) or it has zero-size geometry (empty mesh). Ensure it is a `CSGBox3D`, `MeshInstance3D`, or similar `GeometryInstance3D` subclass with actual geometry.

### Swipe order is wrong (devices before rooms)
Check the scene tree order. Devices must come AFTER rooms in the tree. Use two sibling groups: `RoomsGroup` then `DevicesGroup`.

### Multiple nodes announced at the same position
The engine deduplicates nodes with identical screen bounds (keeps the first one in tree order). If you still see duplicates, ensure nodes don't have the exact same 3D position and size.

### TalkBack announces internal Godot nodes ("Constants", "DirectionalLight3D")
These structural nodes have zero screen bounds and are automatically hidden from TalkBack. If they still appear, they may have non-zero geometry. Set `visible = false` on nodes that should not be accessible.

### Focus jumps to Android bottom bar during swipe
Ensure v12 AAR is correctly integrated. Check logcat:
```bash
adb logcat -s GodotA11yFixer
```
You should see:
```
I GodotA11yFixer: install: CollectionInfoFixer installed (delegate=Delegate)
```
If the fixer is installed, scroll containment is active. If it's not installed, ensure the AAR was properly replaced in the build.

---

## Verification Checklist

- [ ] `adb logcat -s GodotA11yFixer` shows "installed"
- [ ] Touch exploration: hover over a room, TalkBack announces room name
- [ ] Touch exploration: hover over a device pin, TalkBack announces device name
- [ ] Right swipe from Android top bar enters Godot view (first room)
- [ ] Right swipe navigates through all rooms in order
- [ ] Right swipe after last room goes to first device
- [ ] Right swipe navigates through all devices in order
- [ ] Right swipe after last device does NOT exit to Android bottom bar
- [ ] Left swipe reverses the entire order
- [ ] Double-tap on a focused device activates it
- [ ] Room focus rectangle covers the entire room floor area (not a tiny dot)
