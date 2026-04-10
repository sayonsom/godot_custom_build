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

import java.util.ArrayList;
import java.util.HashMap;
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

	/** Build identifier — change on every release to verify the correct engine is loaded. */
	public static final String BUILD_ID = "godot-custom-p0-fix-20260410-016";

	/** Log the build version at class load time so logcat confirms which engine is in use. */
	static {
		Log.i(TAG, "========================================================");
		Log.i(TAG, "  CollectionInfoFixer BUILD_ID: " + BUILD_ID);
		Log.i(TAG, "  P0 fix: TYPE_WINDOW_CONTENT_CHANGED on subtree change");
		Log.i(TAG, "========================================================");
	}

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

	/**
	 * Class names that are KNOWN interactive widgets — these always get
	 * clickable + click action in addition to focusability.
	 */
	private static final Set<String> CLICKABLE_CLASSES = new HashSet<>();
	static {
		CLICKABLE_CLASSES.add("android.widget.Button");
		CLICKABLE_CLASSES.add("android.widget.ImageButton");
		CLICKABLE_CLASSES.add("android.widget.ImageView");
		CLICKABLE_CLASSES.add("android.widget.CheckBox");
		CLICKABLE_CLASSES.add("android.widget.RadioButton");
		CLICKABLE_CLASSES.add("android.widget.ToggleButton");
		CLICKABLE_CLASSES.add("android.widget.Switch");
		CLICKABLE_CLASSES.add("android.widget.EditText");
		CLICKABLE_CLASSES.add("android.widget.SeekBar");
		CLICKABLE_CLASSES.add("android.widget.Spinner");
		CLICKABLE_CLASSES.add("android.widget.ProgressBar");
	}

	/**
	 * Class names that are pure containers / structural — these should NOT
	 * be individually focusable (TalkBack navigates their children instead).
	 */
	private static final Set<String> CONTAINER_CLASSES = new HashSet<>();
	static {
		CONTAINER_CLASSES.add("android.widget.ListView");
		CONTAINER_CLASSES.add("android.widget.GridView");
		CONTAINER_CLASSES.add("android.widget.ScrollView");
		CONTAINER_CLASSES.add("android.widget.HorizontalScrollView");
		CONTAINER_CLASSES.add("android.widget.FrameLayout");
		CONTAINER_CLASSES.add("android.widget.LinearLayout");
		CONTAINER_CLASSES.add("android.widget.RelativeLayout");
		CONTAINER_CLASSES.add("android.widget.TableLayout");
		CONTAINER_CLASSES.add("android.widget.TableRow");
		CONTAINER_CLASSES.add("android.view.ViewGroup");
	}

	/**
	 * Patches an AccessibilityNodeInfo to fix multiple TalkBack issues:
	 *
	 * 1. CollectionInfo NPE — add empty CollectionInfo for collection class names
	 * 2. Focusability — make ALL nodes with content focusable for TalkBack
	 * 3. Zero-bounds nodes — hide nodes with zero-size bounds that block navigation
	 *
	 * KEY CHANGE (build 002): Instead of a whitelist of FOCUSABLE_CLASSES,
	 * we now use a CONTENT-BASED approach: any virtual node that has a label
	 * (text or contentDescription) and valid bounds is made focusable and
	 * important. This fixes the bug where dynamically added nodes with
	 * class names like "android.view.View" were skipped by TalkBack because
	 * they weren't in the whitelist.
	 *
	 * AccessKit maps Godot roles to Android class names, but many roles
	 * map to generic classes (View, ViewGroup) that the old whitelist
	 * didn't cover. The new approach: if AccessKit gave it a label, the
	 * developer intended it to be accessible, so make it navigable.
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

		// Fix 2: CONTENT-BASED FOCUSABILITY
		//
		// AccessKit always emits importantForAccessibility=false on its
		// virtual nodes. Samsung's TalkBack linear navigation (1-finger
		// swipe right/left) uses isImportantForAccessibility() to build
		// the traversal list and SKIPS nodes where this flag is false.
		//
		// Old approach: whitelist of known widget class names.
		// Problem: dynamically added nodes often have class="android.view.View"
		// or other generic classes that weren't in the whitelist.
		//
		// New approach: ANY node that has meaningful content (label or
		// contentDescription) is made focusable, unless it's a pure container.
		// This covers all AccessKit roles regardless of their Android class mapping.

		boolean hasContent = false;
		CharSequence text = info.getText();
		CharSequence desc = info.getContentDescription();
		if ((text != null && text.length() > 0) || (desc != null && desc.length() > 0)) {
			hasContent = true;
		}

		// Also treat nodes that already have actions (click, etc.) as having content —
		// AccessKit adds actions for interactive Godot controls.
		if (!hasContent) {
			// Check if AccessKit assigned any meaningful actions
			List<AccessibilityNodeInfo.AccessibilityAction> actions = info.getActionList();
			if (actions != null) {
				for (AccessibilityNodeInfo.AccessibilityAction action : actions) {
					int id = action.getId();
					if (id == AccessibilityNodeInfo.ACTION_CLICK ||
						id == AccessibilityNodeInfo.ACTION_LONG_CLICK ||
						id == AccessibilityNodeInfo.ACTION_SELECT ||
						id == AccessibilityNodeInfo.ACTION_EXPAND ||
						id == AccessibilityNodeInfo.ACTION_COLLAPSE) {
						hasContent = true;
						break;
					}
				}
			}
		}

		boolean isContainer = CONTAINER_CLASSES.contains(cls);

		if (hasContent && !isContainer) {
			info.setFocusable(true);
			info.setEnabled(true);
			info.setVisibleToUser(true);

			// Mark as important so TalkBack includes it in swipe traversal
			if (android.os.Build.VERSION.SDK_INT >= 24) {
				info.setImportantForAccessibility(true);
			}

			// Known interactive widgets get clickable flag
			if (CLICKABLE_CLASSES.contains(cls)) {
				info.setClickable(true);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CLICK);
			}

			// All focusable nodes need the accessibility focus actions for TalkBack
			info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);
			info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CLEAR_ACCESSIBILITY_FOCUS);

			Log.i(TAG, "FIX-FOCUS: class=" + cls + " label=\"" +
				(desc != null ? desc : text) + "\" -> focusable=true, important=true");
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
					Log.i(TAG, "install: already installed");
					return;
				}
				installWrapper(view, existingDelegate);
			}
		}, 1000); // Initial 1-second delay
	}

	private static void installWrapper(View view, View.AccessibilityDelegate delegate) {
		view.setAccessibilityDelegate(new FixingDelegate(delegate));
		Log.i(TAG, "install: CollectionInfoFixer installed (BUILD_ID=" + BUILD_ID +
			", delegate=" + (delegate != null ? delegate.getClass().getSimpleName() : "null") +
			", P0_fix=TYPE_WINDOW_CONTENT_CHANGED)");

		// BUILD 015: Send synthetic hover events across a grid of screen
		// coordinates to force AccessKit's InjectingAdapter to create VIDs
		// for all nodes with bounds in the visible area. AccessKit creates
		// VIDs lazily — only when TalkBack queries specific coordinates.
		// By hovering across the entire view, we force VID creation for
		// all visible nodes at startup.
		view.postDelayed(() -> {
			try {
				int width = view.getWidth();
				int height = view.getHeight();
				if (width <= 0 || height <= 0) {
					Log.w(TAG, "HOVER-SCAN: view has no size yet, retrying in 2s");
					view.postDelayed(() -> performHoverScan(view), 2000);
					return;
				}
				performHoverScan(view);
			} catch (Exception e) {
				Log.w(TAG, "HOVER-SCAN: failed", e);
			}
		}, 3000); // Wait 3s for scene to load
	}

	/**
	 * BUILD 015: Send synthetic hover events across a grid to force
	 * AccessKit to create VIDs for all visible nodes.
	 */
	private static void performHoverScan(View view) {
		try {
			int width = view.getWidth();
			int height = view.getHeight();
			if (width <= 0 || height <= 0) return;

			// Scan a grid of points across the view
			int stepX = Math.max(width / 20, 50);  // 20 columns or 50px steps
			int stepY = Math.max(height / 20, 50); // 20 rows or 50px steps
			int pointCount = 0;

			long now = android.os.SystemClock.uptimeMillis();

			for (int y = 0; y < height; y += stepY) {
				for (int x = 0; x < width; x += stepX) {
					// Create a hover event at this coordinate
					android.view.MotionEvent hover = android.view.MotionEvent.obtain(
						now, now,
						android.view.MotionEvent.ACTION_HOVER_MOVE,
						(float) x, (float) y, 0);
					hover.setSource(android.view.InputDevice.SOURCE_TOUCHSCREEN);

					// Dispatch to the view — AccessKit's delegate will process it
					// and create VIDs for nodes at these coordinates
					view.dispatchGenericMotionEvent(hover);
					hover.recycle();
					pointCount++;
				}
			}

			// Send a final hover exit
			android.view.MotionEvent exit = android.view.MotionEvent.obtain(
				now, now,
				android.view.MotionEvent.ACTION_HOVER_EXIT,
				0f, 0f, 0);
			exit.setSource(android.view.InputDevice.SOURCE_TOUCHSCREEN);
			view.dispatchGenericMotionEvent(exit);
			exit.recycle();

			Log.i(TAG, "HOVER-SCAN: scanned " + pointCount + " points (" +
				width + "x" + height + ", step=" + stepX + "x" + stepY + ")");

			// Now send a content changed event so TalkBack re-queries with new VIDs
			try {
				if (view.getParent() != null) {
					android.view.accessibility.AccessibilityEvent event =
						android.view.accessibility.AccessibilityEvent.obtain(
							android.view.accessibility.AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
					event.setContentChangeTypes(
						android.view.accessibility.AccessibilityEvent.CONTENT_CHANGE_TYPE_SUBTREE);
					event.setPackageName(view.getContext().getPackageName());
					event.setSource(view, android.view.accessibility.AccessibilityNodeProvider.HOST_VIEW_ID);
					view.getParent().requestSendAccessibilityEvent(view, event);
					Log.i(TAG, "HOVER-SCAN: sent CONTENT_CHANGED after scan");
				}
			} catch (Exception e) {
				Log.w(TAG, "HOVER-SCAN: content-changed event failed", e);
			}
		} catch (Exception e) {
			Log.w(TAG, "HOVER-SCAN: failed", e);
		}
	}

	/**
	 * Wrapper delegate that patches AccessibilityNodeInfo objects.
	 */
	static class FixingDelegate extends View.AccessibilityDelegate {
		private final View.AccessibilityDelegate mOriginal;

		/** Cache the FixingNodeProvider to preserve state across TalkBack queries. */
		private FixingNodeProvider mCachedProvider = null;
		private AccessibilityNodeProvider mCachedOriginal = null;

		/** Track whether we have a provider (set once, avoids JNI calls). */
		private boolean mHasProvider = false;
		private boolean mProviderChecked = false;

		FixingDelegate(View.AccessibilityDelegate original) {
			mOriginal = original;
		}

		@Override
		public AccessibilityNodeProvider getAccessibilityNodeProvider(View host) {
			// SAFETY: All code wrapped in try/catch. Any unhandled Java exception
			// will propagate through JNI into AccessKit's Rust code, which calls
			// .unwrap() and panics with SIGABRT. We MUST catch everything.
			try {
				AccessibilityNodeProvider provider = null;
				if (mOriginal != null) {
					provider = mOriginal.getAccessibilityNodeProvider(host);
				}
				if (provider == null) {
					mHasProvider = false;
					mProviderChecked = true;
					return null;
				}
				mHasProvider = true;
				mProviderChecked = true;

				// BUILD 006 FIX: AccessKit returns a NEW provider object on every
				// call. If we create a new FixingNodeProvider each time, the state
				// (mSeenBounds, mItemIndex, mLastKnownChildCount) resets, causing:
				// - row=0 on every node (CollectionItemInfo never increments)
				// - dedup cache always empty (never detects duplicates)
				// - childCount change detection broken
				//
				// Fix: Create the FixingNodeProvider ONCE. On subsequent calls,
				// just update the inner delegate reference so it queries the
				// latest AccessKit data, while preserving our tracking state.
				if (mCachedProvider == null) {
					mCachedProvider = new FixingNodeProvider(provider, host);
					Log.i(TAG, "getAccessibilityNodeProvider: created FixingNodeProvider (first time)");
				} else {
					// Update the inner delegate to the latest provider instance
					// without resetting our tracking state.
					mCachedProvider.updateDelegate(provider);
				}
				return mCachedProvider;
			} catch (Exception e) {
				Log.e(TAG, "getAccessibilityNodeProvider: CAUGHT exception (preventing Rust panic)", e);
				return null;
			}
		}

		@Override
		public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
			// SAFETY: Wrapped in try/catch to prevent Rust panic.
			try {
				if (mOriginal != null) {
					mOriginal.onInitializeAccessibilityNodeInfo(host, info);
				} else {
					super.onInitializeAccessibilityNodeInfo(host, info);
				}

				// BUILD 005 FIX: Simplified host View handling.
				// Do NOT call back into getAccessibilityNodeProvider() here —
				// that triggers JNI calls into Rust during a callback, which
				// caused the JavaException -> Rust unwrap() -> SIGABRT crash.
				//
				// Instead, use the cached flag from getAccessibilityNodeProvider().
				// If we haven't checked yet, assume provider exists (safe default).
				boolean hasProvider = mProviderChecked ? mHasProvider : true;

				if (hasProvider) {
					// Provider exists — make the host View non-focusable so
					// TalkBack enters the virtual children via the provider.
					info.setFocusable(false);
					info.setClickable(false);
					info.removeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);
					info.removeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CLICK);
					if (android.os.Build.VERSION.SDK_INT >= 24) {
						info.setImportantForAccessibility(true);
					}
					if (android.os.Build.VERSION.SDK_INT >= 28) {
						info.setScreenReaderFocusable(false);
					}
				} else {
					fixNodeInfo(info);
				}
			} catch (Exception e) {
				Log.e(TAG, "onInitializeAccessibilityNodeInfo: CAUGHT exception", e);
			}
		}

		@Override
		public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
			try {
				if (mOriginal != null) {
					mOriginal.onInitializeAccessibilityEvent(host, event);
				} else {
					super.onInitializeAccessibilityEvent(host, event);
				}
			} catch (Exception e) {
				Log.e(TAG, "onInitializeAccessibilityEvent: CAUGHT exception", e);
			}
		}

		@Override
		public boolean dispatchPopulateAccessibilityEvent(View host, AccessibilityEvent event) {
			try {
				if (mOriginal != null) {
					return mOriginal.dispatchPopulateAccessibilityEvent(host, event);
				}
				return super.dispatchPopulateAccessibilityEvent(host, event);
			} catch (Exception e) {
				Log.e(TAG, "dispatchPopulateAccessibilityEvent: CAUGHT exception", e);
				return false;
			}
		}

		@Override
		public void sendAccessibilityEvent(View host, int eventType) {
			try {
				if (mOriginal != null) {
					mOriginal.sendAccessibilityEvent(host, eventType);
				} else {
					super.sendAccessibilityEvent(host, eventType);
				}
			} catch (Exception e) {
				Log.e(TAG, "sendAccessibilityEvent: CAUGHT exception", e);
			}
		}

		@Override
		public void sendAccessibilityEventUnchecked(View host, AccessibilityEvent event) {
			try {
				if (mOriginal != null) {
					mOriginal.sendAccessibilityEventUnchecked(host, event);
				} else {
					super.sendAccessibilityEventUnchecked(host, event);
				}
			} catch (Exception e) {
				Log.e(TAG, "sendAccessibilityEventUnchecked: CAUGHT exception", e);
			}
		}

		@Override
		public boolean onRequestSendAccessibilityEvent(android.view.ViewGroup host, View child, AccessibilityEvent event) {
			try {
				if (mOriginal != null) {
					return mOriginal.onRequestSendAccessibilityEvent(host, child, event);
				}
				return super.onRequestSendAccessibilityEvent(host, child, event);
			} catch (Exception e) {
				Log.e(TAG, "onRequestSendAccessibilityEvent: CAUGHT exception", e);
				return false;
			}
		}

		@Override
		public boolean performAccessibilityAction(View host, int action, Bundle args) {
			try {
				Log.i(TAG, "DELEGATE-ACTION: action=" + action + " (VIEW-LEVEL)");
				if (mOriginal != null) {
					return mOriginal.performAccessibilityAction(host, action, args);
				}
				return super.performAccessibilityAction(host, action, args);
			} catch (Exception e) {
				Log.e(TAG, "performAccessibilityAction: CAUGHT exception", e);
				return false;
			}
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
		private volatile AccessibilityNodeProvider mDelegate;
		private final View mHost;

		/**
		 * BUILD 006: Update the inner delegate without resetting state.
		 * AccessKit returns a new provider object on every call to
		 * getAccessibilityNodeProvider(). We swap the delegate reference
		 * so queries go to the latest data, but our tracking state
		 * (mSeenBounds, mItemIndex, mLastKnownChildCount) persists.
		 */
		void updateDelegate(AccessibilityNodeProvider newDelegate) {
			mDelegate = newDelegate;
		}

		// Counter for CollectionItemInfo row indices, reset on tree refresh.
		private int mItemIndex = 0;

		// BUILD 007: Track whether we're in a tree walk.
		private boolean mInTreeWalk = false;

		// BUILD 010: Flat list of ALL focusable leaf VIDs collected during
		// tree walk. Used to build the flat navigation order for TalkBack.
		private final List<Integer> mFlatFocusableVids = new ArrayList<>();

		// BUILD 010: Map from VID -> row index in the flat list.
		// When TalkBack queries an individual node, we look up its row here.
		private final HashMap<Integer, Integer> mVidToRow = new HashMap<>();

		// P0 FIX: Track the last known child count so we can detect when the
		// tree structure changes (nodes added/removed dynamically) and notify
		// TalkBack to rebuild its traversal list.
		private int mLastKnownChildCount = -1;

		// P0 FIX: Track the last known set of focusable child virtual IDs so
		// we detect structural changes even when total child count doesn't change
		// (e.g., one node removed and another added in the same frame).
		private int mLastKnownFocusableCount = -1;

		FixingNodeProvider(AccessibilityNodeProvider delegate, View host) {
			mDelegate = delegate;
			mHost = host;
		}

		/**
		 * BUILD 010: Recursively walk the AccessKit tree and collect all
		 * focusable leaf node VIDs into a flat list. This is the core of
		 * the Java-side tree flattening.
		 */
		private void collectFocusableLeaves(int vid, Set<Integer> visited) {
			if (visited.contains(vid)) return;
			visited.add(vid);

			AccessibilityNodeInfo node = null;
			try {
				node = mDelegate.createAccessibilityNodeInfo(vid);
			} catch (Exception e) {
				return;
			}
			if (node == null) return;

			android.graphics.Rect bounds = new android.graphics.Rect();
			node.getBoundsInScreen(bounds);
			int childCount = node.getChildCount();
			boolean hasValidBounds = bounds.width() > 0 && bounds.height() > 0;

			// Check if this node has content (label or description)
			CharSequence text = node.getText();
			CharSequence desc = node.getContentDescription();
			boolean hasContent = (text != null && text.length() > 0) ||
				(desc != null && desc.length() > 0);

			// A focusable leaf: has valid bounds AND (has content OR is natively focusable)
			if (hasValidBounds && (hasContent || node.isFocusable())) {
				mFlatFocusableVids.add(vid);
			}

			// Recurse into children regardless — we scan by trying sequential VIDs
			// that are children of this node. AccessKit uses sequential int IDs
			// so we try VIDs beyond the ones we've seen.
			node.recycle();

			// Also try to find children by scanning VIDs.
			// Since AccessKit uses dense integer VIDs, children of vid=N
			// typically have higher VIDs. We scan a range around the current max.
		}

		/** Track the highest VID we've ever seen to optimize scan range. */
		private int mHighestVidSeen = 99;

		/**
		 * BUILD 012: Adaptive full tree scan — collect ALL focusable nodes.
		 * Scans VIDs from 0 up to mHighestVidSeen + 200, extending the
		 * range whenever new nodes are found. Stops early if we hit 200
		 * consecutive null VIDs (gap = no more nodes beyond this point).
		 */
		private void rebuildFlatList() {
			mFlatFocusableVids.clear();
			mVidToRow.clear();

			int scanLimit = mHighestVidSeen + 200;
			int consecutiveNulls = 0;
			int scanned = 0;

			for (int vid = 0; vid <= scanLimit && consecutiveNulls < 200; vid++) {
				AccessibilityNodeInfo node = null;
				try {
					node = mDelegate.createAccessibilityNodeInfo(vid);
				} catch (Exception e) {
					consecutiveNulls++;
					continue;
				}
				if (node == null) {
					consecutiveNulls++;
					continue;
				}

				consecutiveNulls = 0; // Reset gap counter
				scanned++;

				// Track highest VID to expand future scans
				if (vid > mHighestVidSeen) {
					mHighestVidSeen = vid;
					scanLimit = mHighestVidSeen + 200; // Extend range
				}

				android.graphics.Rect bounds = new android.graphics.Rect();
				node.getBoundsInScreen(bounds);
				boolean hasValidBounds = bounds.width() > 0 && bounds.height() > 0;

				CharSequence text = node.getText();
				CharSequence desc = node.getContentDescription();
				boolean hasContent = (text != null && text.length() > 0) ||
					(desc != null && desc.length() > 0);

				if (hasValidBounds && (hasContent || node.isFocusable())) {
					mFlatFocusableVids.add(vid);
				}

				node.recycle();
			}

			// Build the vid -> row map
			for (int i = 0; i < mFlatFocusableVids.size(); i++) {
				mVidToRow.put(mFlatFocusableVids.get(i), i);
			}

			// Log summary (not every node — too noisy with 2751 nodes)
			StringBuilder summary = new StringBuilder();
			for (int i = 0; i < Math.min(mFlatFocusableVids.size(), 20); i++) {
				if (i > 0) summary.append(", ");
				summary.append("vid=").append(mFlatFocusableVids.get(i));
			}
			if (mFlatFocusableVids.size() > 20) {
				summary.append(", ... (").append(mFlatFocusableVids.size() - 20).append(" more)");
			}

			Log.i(TAG, "FLAT-TOTAL: " + mFlatFocusableVids.size() +
				" focusable nodes found (scanned " + scanned + " nodes, " +
				"VIDs 0-" + mHighestVidSeen + ") [" + summary + "]");
		}

		/**
		 * P0 FIX: Notify Android's accessibility framework that the virtual
		 * tree structure has changed. This forces TalkBack to re-query the
		 * tree and rebuild its traversal list, fixing stale-focus issues
		 * when nodes are dynamically added or removed.
		 */
		private void notifySubtreeChanged() {
			if (mHost == null || mHost.getParent() == null) return;
			mHost.post(() -> {
				try {
					if (mHost.getParent() == null) return;
					AccessibilityEvent event = AccessibilityEvent.obtain(
						AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
					event.setContentChangeTypes(
						AccessibilityEvent.CONTENT_CHANGE_TYPE_SUBTREE);
					event.setPackageName(mHost.getContext().getPackageName());
					event.setSource(mHost, HOST_VIEW_ID);
					mHost.getParent().requestSendAccessibilityEvent(mHost, event);
					Log.i(TAG, "P0: sent TYPE_WINDOW_CONTENT_CHANGED / SUBTREE — " +
						"TalkBack will re-query virtual tree");
				} catch (Exception e) {
					Log.w(TAG, "P0: failed to send content-changed event", e);
				}
			});
		}

		@Override
		public AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualViewId) {
			// SAFETY: Everything wrapped in try/catch. Any exception that escapes
			// to AccessKit's Rust code causes SIGABRT via unwrap() panic.
			try {
			AccessibilityNodeInfo info = mDelegate.createAccessibilityNodeInfo(virtualViewId);
			if (info == null) {
				// DEBUG: AccessKit returned null for this virtual ID — the node
				// may have been removed between TalkBack's query and now.
				Log.w(TAG, "DEBUG: createAccessibilityNodeInfo(" + virtualViewId +
					") returned NULL — node may have been removed mid-traversal");
				return null;
			}

			if (virtualViewId == AccessibilityNodeProvider.HOST_VIEW_ID) {
				// === ROOT NODE (the container) ===
				mItemIndex = 0;
				mInTreeWalk = true;

				// BUILD 010: Rebuild the flat focusable list by scanning ALL VIDs.
				// This is the Java-side tree flattening — we collect every
				// focusable node regardless of nesting depth and present them
				// as a flat ordered list to TalkBack.
				rebuildFlatList();

				// Make the root NOT directly focusable
				info.setFocusable(false);
				info.setClickable(false);
				info.removeAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);

				// Mark as a ListView collection with the FLAT count
				info.setClassName("android.widget.ListView");
				int flatCount = mFlatFocusableVids.size();
				info.setCollectionInfo(
					AccessibilityNodeInfo.CollectionInfo.obtain(
						Math.max(flatCount, 1), 1, false));

				// BUILD 010: Add ALL flat focusable VIDs as direct children
				// of the ROOT node. This overrides AccessKit's nested hierarchy
				// and presents a flat list to TalkBack.
				for (int flatVid : mFlatFocusableVids) {
					info.addChild(mHost, flatVid);
				}

				// Mark as scrollable for containment
				info.setScrollable(true);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_FORWARD);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_BACKWARD);

				if (android.os.Build.VERSION.SDK_INT >= 24) {
					info.setImportantForAccessibility(true);
				}
				if (android.os.Build.VERSION.SDK_INT >= 28) {
					info.setScreenReaderFocusable(false);
				}

				fixNodeInfo(info);

				// P0 FIX: Detect flat list changes and notify TalkBack
				if (mLastKnownChildCount >= 0 && flatCount != mLastKnownChildCount) {
					Log.i(TAG, "P0: flat focusable count changed " + mLastKnownChildCount +
						" -> " + flatCount + " — notifying TalkBack");
					notifySubtreeChanged();
				}
				mLastKnownChildCount = flatCount;

				return info;
			}

			// === CHILD VIRTUAL NODE ===
			fixNodeInfo(info);

			// BUILD 010: Use the pre-built flat row map for CollectionItemInfo.
			// This ensures every focusable node gets a STABLE row index that
			// doesn't depend on query order or tree walk state.
			Integer row = mVidToRow.get(virtualViewId);
			if (row != null) {
				info.setCollectionItemInfo(
					AccessibilityNodeInfo.CollectionItemInfo.obtain(
						row, 1, 0, 1, false));

				// Set traversal order so TalkBack knows prev/next
				if (row > 0) {
					int prevVid = mFlatFocusableVids.get(row - 1);
					info.setTraversalAfter(mHost, prevVid);
				}
				if (row < mFlatFocusableVids.size() - 1) {
					int nextVid = mFlatFocusableVids.get(row + 1);
					info.setTraversalBefore(mHost, nextVid);
				}
			}

			return info;
		} catch (Exception e) {
			Log.e(TAG, "createAccessibilityNodeInfo(" + virtualViewId + "): CAUGHT exception (preventing Rust panic)", e);
			return null;
		}
		}

		@Override
		public boolean performAction(int virtualViewId, int action, Bundle arguments) {
		 try {
			// DEBUG: log ALL actions TalkBack sends — this reveals navigation patterns
			String actionName;
			switch (action) {
				case AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS: actionName = "A11Y_FOCUS"; break;
				case AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS: actionName = "CLEAR_A11Y_FOCUS"; break;
				case AccessibilityNodeInfo.ACTION_CLICK: actionName = "CLICK"; break;
				case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD: actionName = "SCROLL_FORWARD"; break;
				case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD: actionName = "SCROLL_BACKWARD"; break;
				case AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY: actionName = "NEXT_GRANULARITY"; break;
				case AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY: actionName = "PREV_GRANULARITY"; break;
				default: actionName = "ACTION_" + action; break;
			}
			Log.i(TAG, "DEBUG-ACTION: vid=" + virtualViewId +
				" action=" + actionName +
				(virtualViewId == AccessibilityNodeProvider.HOST_VIEW_ID ? " (ROOT)" : ""));

			// Intercept scroll actions on the root to keep TalkBack inside.
			// When TalkBack reaches the last virtual node and the user swipes
			// right, TalkBack calls ACTION_SCROLL_FORWARD on the container.
			// Returning true tells TalkBack "I handled the scroll" — TalkBack
			// then re-queries for focusable nodes and stays inside.
			if (virtualViewId == AccessibilityNodeProvider.HOST_VIEW_ID) {
				if (action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD ||
					action == AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) {
					Log.i(TAG, "P0-INTERCEPT: scroll " +
						(action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD ?
							"FORWARD" : "BACKWARD") + " on root — keeping focus in Godot view");
					return true;
				}
			}
			boolean result = mDelegate.performAction(virtualViewId, action, arguments);
			// DEBUG: log if AccessKit couldn't handle the action (potential stale node)
			if (!result) {
				Log.w(TAG, "DEBUG-ACTION-FAIL: vid=" + virtualViewId +
					" action=" + actionName + " returned false — possible stale node");
			}
			return result;
		 } catch (Exception e) {
			Log.e(TAG, "performAction(" + virtualViewId + "," + action + "): CAUGHT exception", e);
			return false;
		 }
		}

		@Override
		public List<AccessibilityNodeInfo> findAccessibilityNodeInfosByText(
				String text, int virtualViewId) {
			try {
				return mDelegate.findAccessibilityNodeInfosByText(text, virtualViewId);
			} catch (Exception e) {
				Log.e(TAG, "findAccessibilityNodeInfosByText: CAUGHT exception", e);
				return null;
			}
		}

		@Override
		public AccessibilityNodeInfo findFocus(int focus) {
		 try {
			AccessibilityNodeInfo info = mDelegate.findFocus(focus);
			if (info == null) {
				// DEBUG: TalkBack asked where focus is but AccessKit says "nowhere"
				// This happens when the focused node was removed from the tree.
				Log.w(TAG, "DEBUG-FOCUS: findFocus(" +
					(focus == AccessibilityNodeInfo.FOCUS_ACCESSIBILITY ? "A11Y" : "INPUT") +
					") returned NULL — focused node may have been removed");
			} else {
				fixNodeInfo(info);
				CharSequence label = info.getText();
				if (label == null) label = info.getContentDescription();
				Log.i(TAG, "DEBUG-FOCUS: findFocus(" +
					(focus == AccessibilityNodeInfo.FOCUS_ACCESSIBILITY ? "A11Y" : "INPUT") +
					") -> label=\"" + (label != null ? label : "<no-label>") +
					"\" class=" + info.getClassName());
			}
			return info;
		 } catch (Exception e) {
			Log.e(TAG, "findFocus: CAUGHT exception", e);
			return null;
		 }
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
