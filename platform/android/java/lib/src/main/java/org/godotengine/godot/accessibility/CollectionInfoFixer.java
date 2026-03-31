/**************************************************************************/
/*  CollectionInfoFixer.java                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

package org.godotengine.godot.accessibility;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Fixes TalkBack crashes caused by AccessKit not setting CollectionInfo
 * on nodes with collection-type Android class names.
 *
 * AccessKit's Android adapter sets className to android.widget.ListView,
 * GridView, etc. based on the AccessKit role, but never calls
 * setCollectionInfo() on the AccessibilityNodeInfo. TalkBack then calls
 * CollectionInfo.getItemCount() which NPEs on the null reference.
 *
 * This class wraps AccessKit's AccessibilityDelegate and patches every
 * AccessibilityNodeInfo returned by the provider.
 */
public class CollectionInfoFixer {
	private static final String TAG = "GodotA11yFixer";

	/** Class names that TalkBack treats as collections (requires CollectionInfo). */
	private static final Set<String> COLLECTION_CLASSES = new HashSet<>();
	static {
		COLLECTION_CLASSES.add("android.widget.ListView");
		COLLECTION_CLASSES.add("android.widget.GridView");
		COLLECTION_CLASSES.add("android.widget.TabWidget");
		COLLECTION_CLASSES.add("android.widget.Spinner");
		COLLECTION_CLASSES.add("android.widget.RadioGroup");
		COLLECTION_CLASSES.add("android.widget.AbsListView");
		COLLECTION_CLASSES.add("android.widget.ExpandableListView");
		COLLECTION_CLASSES.add("android.widget.NumberPicker");
		COLLECTION_CLASSES.add("android.widget.StackView");
		COLLECTION_CLASSES.add("android.widget.AdapterView");
		COLLECTION_CLASSES.add("android.widget.RecyclerView");
		COLLECTION_CLASSES.add("androidx.recyclerview.widget.RecyclerView");
		COLLECTION_CLASSES.add("android.widget.TableLayout");
	}

	/** Class names that should be focusable and clickable for TalkBack navigation. */
	private static final Set<String> FOCUSABLE_CLASSES = new HashSet<>();
	static {
		FOCUSABLE_CLASSES.add("android.widget.Button");
		FOCUSABLE_CLASSES.add("android.widget.ImageButton");
		FOCUSABLE_CLASSES.add("android.widget.ImageView");
		FOCUSABLE_CLASSES.add("android.widget.CheckBox");
		FOCUSABLE_CLASSES.add("android.widget.RadioButton");
		FOCUSABLE_CLASSES.add("android.widget.ToggleButton");
		FOCUSABLE_CLASSES.add("android.widget.Switch");
		FOCUSABLE_CLASSES.add("android.widget.EditText");
		FOCUSABLE_CLASSES.add("android.widget.SeekBar");
		FOCUSABLE_CLASSES.add("android.widget.Spinner");
		FOCUSABLE_CLASSES.add("android.widget.ProgressBar");
		FOCUSABLE_CLASSES.add("android.widget.TextView");
	}

	/**
	 * Patches an AccessibilityNodeInfo to fix multiple TalkBack issues:
	 *
	 * 1. CollectionInfo NPE — add empty CollectionInfo for collection class names
	 * 2. Focusability — make interactive nodes (Button, ImageView, etc.) focusable
	 *    and clickable so TalkBack can navigate to them with left/right swipe
	 * 3. Zero-bounds nodes — hide nodes with zero-size bounds that block navigation
	 */
	public static void fixNodeInfo(AccessibilityNodeInfo info) {
		if (info == null) return;
		CharSequence className = info.getClassName();
		if (className == null) return;
		String cls = className.toString();

		// Fix 1: CollectionInfo NPE
		if (COLLECTION_CLASSES.contains(cls) && info.getCollectionInfo() == null) {
			info.setCollectionInfo(
				AccessibilityNodeInfo.CollectionInfo.obtain(0, 1, false));
		}

		// Fix 2: Make interactive nodes focusable, clickable, and importantly
		// mark them as important for accessibility.
		//
		// AccessKit sets the correct className but always emits
		// importantForAccessibility=false on its virtual nodes. Samsung's
		// TalkBack linear navigation (1-finger swipe right/left) uses
		// isImportantForAccessibility() to build the traversal list and
		// SKIPS nodes where this flag is false — even if they have
		// ACTION_ACCESSIBILITY_FOCUS. Touch exploration (finger hover) uses
		// a coordinate lookup and ignores this flag, which is why touch
		// worked but swipe did not.
		//
		// Setting importantForAccessibility=true on interactive nodes is the
		// key fix that enables 1-finger swipe to navigate between them.
		if (FOCUSABLE_CLASSES.contains(cls)) {
			info.setFocusable(true);
			info.setEnabled(true);
			info.setVisibleToUser(true);
			// Mark as important so TalkBack includes it in swipe traversal list
			if (android.os.Build.VERSION.SDK_INT >= 24) {
				info.setImportantForAccessibility(true);
			}
			// Buttons and interactive widgets should be clickable
			if (!cls.equals("android.widget.TextView")) {
				info.setClickable(true);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CLICK);
			}
			// All focusable nodes need the accessibility focus action for TalkBack
			info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);
			info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CLEAR_ACCESSIBILITY_FOCUS);
		}

		// Fix 3: Hide zero-bounds nodes from TalkBack.
		// These are structural/internal Godot nodes (like "Constants",
		// "DirectionalLight3D") that have no visual representation.
		// If TalkBack focuses one of these, swipe navigation breaks
		// because TalkBack can't compute spatial ordering from (0,0,0,0).
		android.graphics.Rect bounds = new android.graphics.Rect();
		info.getBoundsInScreen(bounds);
		if (bounds.width() <= 0 || bounds.height() <= 0) {
			info.setVisibleToUser(false);
			info.setFocusable(false);
			// Also mark as NOT important so TalkBack skips it in traversal
			if (android.os.Build.VERSION.SDK_INT >= 24) {
				info.setImportantForAccessibility(false);
			}
			// Clear any accessibility focus if this zero-bounds node had it
			info.removeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);
		}
	}

	/**
	 * Installs a patching AccessibilityDelegate that wraps whatever
	 * AccessKit sets. Uses a delayed approach: we keep checking until
	 * AccessKit's delegate is present, then wrap it.
	 */
	public static void install(final View view) {
		if (view == null) return;
		// Post with a delay to let AccessKit set up its delegate first.
		// Retry up to 10 times with 500ms intervals.
		view.postDelayed(new Runnable() {
			int attempts = 0;

			@Override
			public void run() {
				attempts++;
				View.AccessibilityDelegate existingDelegate = getDelegate(view);
				if (existingDelegate == null) {
					if (attempts < 10) {
						view.postDelayed(this, 500);
					} else {
						Log.w(TAG, "install: AccessKit delegate not found after " + attempts + " attempts, installing blind wrapper");
						installWrapper(view, null);
					}
					return;
				}
				// Check if it's already our wrapper
				if (existingDelegate instanceof FixingDelegate) {
					Log.d(TAG, "install: already installed");
					return;
				}
				installWrapper(view, existingDelegate);
			}
		}, 1000); // Initial 1-second delay
	}

	private static void installWrapper(View view, View.AccessibilityDelegate delegate) {
		view.setAccessibilityDelegate(new FixingDelegate(delegate));
		Log.i(TAG, "install: CollectionInfoFixer installed (delegate=" +
			(delegate != null ? delegate.getClass().getSimpleName() : "null") + ")");
	}

	/**
	 * Wrapper delegate that patches AccessibilityNodeInfo objects.
	 */
	static class FixingDelegate extends View.AccessibilityDelegate {
		private final View.AccessibilityDelegate mOriginal;

		FixingDelegate(View.AccessibilityDelegate original) {
			mOriginal = original;
		}

		@Override
		public AccessibilityNodeProvider getAccessibilityNodeProvider(View host) {
			// Get the provider from the ORIGINAL delegate (AccessKit's).
			// Do NOT call host.getAccessibilityNodeProvider() — that would
			// recurse back into this delegate and cause infinite recursion.
			AccessibilityNodeProvider provider = null;
			if (mOriginal != null) {
				provider = mOriginal.getAccessibilityNodeProvider(host);
			}
			if (provider == null) {
				return null;
			}
			// Wrap it to fix CollectionInfo on every node returned
			return new FixingNodeProvider(provider);
		}

		@Override
		public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
			if (mOriginal != null) {
				mOriginal.onInitializeAccessibilityNodeInfo(host, info);
			} else {
				super.onInitializeAccessibilityNodeInfo(host, info);
			}
			fixNodeInfo(info);
		}

		@Override
		public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
			if (mOriginal != null) {
				mOriginal.onInitializeAccessibilityEvent(host, event);
			} else {
				super.onInitializeAccessibilityEvent(host, event);
			}
		}

		@Override
		public boolean dispatchPopulateAccessibilityEvent(View host, AccessibilityEvent event) {
			if (mOriginal != null) {
				return mOriginal.dispatchPopulateAccessibilityEvent(host, event);
			}
			return super.dispatchPopulateAccessibilityEvent(host, event);
		}

		@Override
		public void sendAccessibilityEvent(View host, int eventType) {
			if (mOriginal != null) {
				mOriginal.sendAccessibilityEvent(host, eventType);
			} else {
				super.sendAccessibilityEvent(host, eventType);
			}
		}

		@Override
		public void sendAccessibilityEventUnchecked(View host, AccessibilityEvent event) {
			if (mOriginal != null) {
				mOriginal.sendAccessibilityEventUnchecked(host, event);
			} else {
				super.sendAccessibilityEventUnchecked(host, event);
			}
		}

		@Override
		public boolean onRequestSendAccessibilityEvent(android.view.ViewGroup host, View child, AccessibilityEvent event) {
			if (mOriginal != null) {
				return mOriginal.onRequestSendAccessibilityEvent(host, child, event);
			}
			return super.onRequestSendAccessibilityEvent(host, child, event);
		}

		@Override
		public boolean performAccessibilityAction(View host, int action, Bundle args) {
			if (mOriginal != null) {
				return mOriginal.performAccessibilityAction(host, action, args);
			}
			return super.performAccessibilityAction(host, action, args);
		}
	}

	/**
	 * Wraps an AccessibilityNodeProvider to fix CollectionInfo on every node.
	 *
	 * Key behaviors:
	 * 1. The ROOT virtual node (HOST_VIEW_ID) is made non-focusable and marked
	 *    as a ListView collection. TalkBack enters children directly and treats
	 *    them as a group — navigating predictably within the Godot view.
	 *
	 * 2. Child virtual nodes get CollectionItemInfo with sequential row indices.
	 *    TalkBack announces "Item X of Y" and knows the navigation order.
	 *
	 * 3. Duplicate-bounds deduplication: when multiple virtual nodes share the
	 *    exact same screen bounds (e.g., IconBase, ModelBase, DeviceObjectInfo,
	 *    InputManager all stacked at the same pixel position), only the FIRST
	 *    one remains focusable. The rest are hidden. This prevents TalkBack from
	 *    "randomly" cycling through invisible duplicates.
	 *
	 * 4. The root node handles ACTION_SCROLL_FORWARD / ACTION_SCROLL_BACKWARD,
	 *    returning true to keep TalkBack inside the Godot view when the user
	 *    reaches the last/first item and swipes again.
	 */
	static class FixingNodeProvider extends AccessibilityNodeProvider {
		private final AccessibilityNodeProvider mDelegate;

		// Track bounds we've already seen in this accessibility pass.
		// Key = "left,top,right,bottom", Value = virtualViewId of first node.
		// Cleared each time the root node is queried (tree refresh).
		private final Set<String> mSeenBounds = ConcurrentHashMap.newKeySet();

		// Counter for CollectionItemInfo row indices, reset on tree refresh.
		private int mItemIndex = 0;

		FixingNodeProvider(AccessibilityNodeProvider delegate) {
			mDelegate = delegate;
		}

		@Override
		public AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualViewId) {
			AccessibilityNodeInfo info = mDelegate.createAccessibilityNodeInfo(virtualViewId);
			if (info == null) return null;

			if (virtualViewId == AccessibilityNodeProvider.HOST_VIEW_ID) {
				// === ROOT NODE (the Mapview container) ===
				// Reset dedup state for this tree-build pass.
				mSeenBounds.clear();
				mItemIndex = 0;

				// Make the root NOT directly focusable — TalkBack should
				// navigate through virtual children, not focus the container.
				info.setFocusable(false);
				info.setClickable(false);
				info.removeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);

				// Mark as a ListView collection so TalkBack treats children
				// as an ordered group and announces "Item X of Y".
				info.setClassName("android.widget.ListView");
				int childCount = info.getChildCount();
				info.setCollectionInfo(
					AccessibilityNodeInfo.CollectionInfo.obtain(
						Math.max(childCount, 20), 1, false));

				// Mark as scrollable — when TalkBack reaches the end of
				// children and calls ACTION_SCROLL_FORWARD, our performAction
				// handler returns true to keep focus inside the Godot view.
				info.setScrollable(true);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_FORWARD);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_BACKWARD);

				if (android.os.Build.VERSION.SDK_INT >= 24) {
					info.setImportantForAccessibility(true);
				}
				if (android.os.Build.VERSION.SDK_INT >= 28) {
					info.setScreenReaderFocusable(false);
				}

				// Still run the standard fixNodeInfo for CollectionInfo NPE fix
				fixNodeInfo(info);
				return info;
			}

			// === CHILD VIRTUAL NODE ===
			fixNodeInfo(info);

			// Dedup: if another node already has these exact bounds, hide this one.
			android.graphics.Rect bounds = new android.graphics.Rect();
			info.getBoundsInScreen(bounds);
			if (bounds.width() > 0 && bounds.height() > 0) {
				String boundsKey = bounds.left + "," + bounds.top + "," +
					bounds.right + "," + bounds.bottom;
				if (!mSeenBounds.add(boundsKey)) {
					// Duplicate bounds — hide this node from TalkBack
					info.setVisibleToUser(false);
					info.setFocusable(false);
					if (android.os.Build.VERSION.SDK_INT >= 24) {
						info.setImportantForAccessibility(false);
					}
					info.removeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);
					return info;
				}
			}

			// Assign CollectionItemInfo with sequential row index so TalkBack
			// knows the navigation order within the Godot view.
			if (info.isVisibleToUser() && info.isFocusable()) {
				int row = mItemIndex++;
				info.setCollectionItemInfo(
					AccessibilityNodeInfo.CollectionItemInfo.obtain(
						row, 1, 0, 1, false));
			}

			return info;
		}

		@Override
		public boolean performAction(int virtualViewId, int action, Bundle arguments) {
			// Intercept scroll actions on the root to keep TalkBack inside.
			// When TalkBack reaches the last virtual node and the user swipes
			// right, TalkBack calls ACTION_SCROLL_FORWARD on the container.
			// Returning true tells TalkBack "I handled the scroll" — TalkBack
			// then re-queries for focusable nodes and stays inside.
			if (virtualViewId == AccessibilityNodeProvider.HOST_VIEW_ID) {
				if (action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD ||
					action == AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) {
					Log.d(TAG, "performAction: intercepted scroll " +
						(action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD ?
							"FORWARD" : "BACKWARD") + " — keeping focus in Godot view");
					return true;
				}
			}
			return mDelegate.performAction(virtualViewId, action, arguments);
		}

		@Override
		public List<AccessibilityNodeInfo> findAccessibilityNodeInfosByText(
				String text, int virtualViewId) {
			return mDelegate.findAccessibilityNodeInfosByText(text, virtualViewId);
		}

		@Override
		public AccessibilityNodeInfo findFocus(int focus) {
			AccessibilityNodeInfo info = mDelegate.findFocus(focus);
			fixNodeInfo(info);
			return info;
		}
	}

	/**
	 * Gets the current AccessibilityDelegate from a View via reflection.
	 * Returns null if none is set or reflection fails.
	 */
	private static View.AccessibilityDelegate getDelegate(View view) {
		try {
			java.lang.reflect.Method m = View.class.getMethod("getAccessibilityDelegate");
			return (View.AccessibilityDelegate) m.invoke(view);
		} catch (Exception e) {
			// getAccessibilityDelegate() was added in API 29
			try {
				java.lang.reflect.Field f = View.class.getDeclaredField("mAccessibilityDelegate");
				f.setAccessible(true);
				return (View.AccessibilityDelegate) f.get(view);
			} catch (Exception e2) {
				return null;
			}
		}
	}
}
