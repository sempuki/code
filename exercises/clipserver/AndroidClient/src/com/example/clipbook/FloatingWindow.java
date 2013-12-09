package com.example.clipbook;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import wei.mark.standout.StandOutWindow;
import wei.mark.standout.constants.StandOutFlags;
import wei.mark.standout.ui.Window;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

public class FloatingWindow extends StandOutWindow {
	private static final String TAG = FloatingWindow.class.getName();
	
	private ClipboardListener clipboardListener = null;
	private class ClipboardListener implements ClipboardManager.OnPrimaryClipChangedListener {
		
		private ClipboardManager clipboard = null;
		public ClipboardListener(ClipboardManager clipboard) {
			this.clipboard = clipboard;
		}
		
		private ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
		
		public void start() {
			// HACK: clipboard notifications are currently broken in android
			scheduler.scheduleAtFixedRate(new Runnable() {
				private ClipData lastClip = null;
				public void run() {
				    if (clipboard.hasPrimaryClip()) {
				    	ClipData currClip = clipboard.getPrimaryClip();
				    	// HACK: Google neglected to overload equals for any of its clipboard classes (nor CharSequence impl)
				    	String currClipText = currClip.getItemAt(0).coerceToText(FloatingWindow.this).toString();
				    	String lastClipText = (lastClip != null)? lastClip.getItemAt(0).coerceToText(FloatingWindow.this).toString() : null;
				    	if (!currClipText.equals(lastClipText)) {
				    		onPrimaryClipChanged();
					    	lastClip = currClip;
				    	}
				    }
				}
			}, 0, 5, TimeUnit.SECONDS);
		}
		
		public void stop() {
			scheduler.shutdown();
		}

		@Override
		public void onPrimaryClipChanged() {
			if (layoutView != null) { 
				ClipData clip = clipboard.getPrimaryClip();
				final ClipData.Item item = clip.getItemAt(0);
				final TextView text = (TextView) layoutView.findViewById(R.id.textView2);
				
				final TextView text2 = (TextView) layoutView.findViewById(R.id.clip2);
				final TextView text3 = (TextView) layoutView.findViewById(R.id.clip3);
				final TextView text4 = (TextView) layoutView.findViewById(R.id.clip4);
				final TextView text5 = (TextView) layoutView.findViewById(R.id.clip5);
				
				text.post(new Runnable() {
					@Override
					public void run() {
						
						text5.setText(text4.getText());
						text4.setText(text3.getText());
						text3.setText(text2.getText());
						text2.setText(text.getText());
						text.setText(item.coerceToText(FloatingWindow.this));
					}
				});
			}
		}
	}
	
	@Override
	public void onCreate() {
		super.onCreate();

	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		
		clipboardListener.stop();
	}

	@Override
	public String getAppName() {
		return "SimpleWindow";
	}

	@Override
	public int getAppIcon() {
		return android.R.drawable.ic_menu_close_clear_cancel;
	}

	private View layoutView = null;
	
	@Override
	public void createAndAttachView(int id, FrameLayout frame) {
		// create a new layout from body.xml
		LayoutInflater inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
		layoutView = inflater.inflate(R.layout.simple, frame, true);
		
		ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
		clipboardListener = new ClipboardListener(clipboard);
		clipboardListener.start();
	}

	// the window will be centered
	@Override
	public StandOutLayoutParams getParams(int id, Window window) {
		return new StandOutLayoutParams(id, 250, 300,
				StandOutLayoutParams.CENTER, StandOutLayoutParams.CENTER);
	}

	// move the window by dragging the view
	@Override
	public int getFlags(int id) {
		return super.getFlags(id) | StandOutFlags.FLAG_BODY_MOVE_ENABLE
				| StandOutFlags.FLAG_WINDOW_FOCUSABLE_DISABLE;
	}

	@Override
	public String getPersistentNotificationMessage(int id) {
		return "Click to close the SimpleWindow";
	}

	@Override
	public Intent getPersistentNotificationIntent(int id) {
		return StandOutWindow.getCloseIntent(this, FloatingWindow.class, id);
	}
}
