package com.example.clipbook;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Toast;


public class MainActivity extends Activity {

	private static final String TAG = MainActivity.class.getName();

	// Lab 18?
	// TODO add login activity

	private static final int MAX_NUM_CLIPS = 5;
	private static final String CLIP_SERIALIZE_PATH = System.getProperty("java.io.tmpdir") + "/clipbook/clips/";

	private ClipSerializer clipSerializer = null;
	private ClipManager clipManager = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.i(TAG, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		clipSerializer = new ClipSerializer(this, CLIP_SERIALIZE_PATH);
		clipManager = new ClipManager(this, MAX_NUM_CLIPS);

		addButtonListeners();
		setupButtonsArray();
		attachClipObserver();
		deserializePersistedClips();
		
		clipManager.start();
	}
	
	@Override
	protected void onStart() {
		super.onStart();
		clipManager.wake();
	}

	@Override
	protected void onStop() {
		clipManager.sleep();
		super.onStop();
	}
	
	@Override
	protected void onDestroy() {
		clipManager.dispose();
		super.onDestroy();
	}

	private static final int RESULT_SETTINGS = 1;

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.settings, menu);
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menu_settings:
			startActivityForResult(new Intent(this, SettingsActivity.class), RESULT_SETTINGS);
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		switch (requestCode) {
		case RESULT_SETTINGS:
			clipManager.restart();
			break;
		}
	}

	private ArrayList<RadioButton> clipButtons = null;

	private void setupButtonsArray() {
		clipButtons = new ArrayList<RadioButton>(MAX_NUM_CLIPS);

		// Generate reference to each clip/ radio button.
		// We cannot assign the R values dynamically

		clipButtons.add((RadioButton) findViewById(R.id.clipView0));
		clipButtons.add((RadioButton) findViewById(R.id.clipView1));
		clipButtons.add((RadioButton) findViewById(R.id.clipView2));
		clipButtons.add((RadioButton) findViewById(R.id.clipView3));
		clipButtons.add((RadioButton) findViewById(R.id.clipView4));
	}
	
	private void deserializePersistedClips() {
        List<File> currFiles = clipSerializer.scanSerializedPath();
        for (File file : currFiles) {
        	Log.i(TAG, "persisted files: " + file);
        	ClipData clip = clipSerializer.deserialize(file);
        	clipPersistence.put(clip, file);
        	clipManager.notify(clip);
        }
	}
	
	private final Map<ClipData, File> clipPersistence = new HashMap<ClipData, File>();
	
	private class ClipObserver implements Observer<List<ClipData>> {
		@Override
		public void observe(final List<ClipData> clips) {
			// In new thread, update the clips to match current clip history
			clipButtons.get(0).post(new Runnable() {
				@Override
				public void run() {
					ClipData clip;
					CharSequence text;
					int visibility;
					
					for (int i=0; i < MAX_NUM_CLIPS; ++i) {
						if (i < clips.size()) {
							clip = clips.get(i);
							text = clip.getItemAt(0).coerceToText(MainActivity.this);
							visibility = View.VISIBLE;
							
							if (clipPersistence.containsKey(clip) == false) {
								clipPersistence.put(clip, clipSerializer.serialize(clip));
								clipManager.sendToWirelessClipboard(clip);
								sendNotification(text);
							}
						} else {
							clip = null;
							text = null;
							visibility = View.GONE;
						}
						
						clipButtons.get(i).setTag(clip);
						clipButtons.get(i).setText(text);
						clipButtons.get(i).setVisibility(visibility);
					}
				}
			});
		}
	}

	private Notification notification = null;
	private NotificationManager notificationManager = null;
	private static final int mId = 1;
	
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	private void sendNotification(CharSequence clip) {		
		
		notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		notificationManager.cancel(mId);
				
		notification = new NotificationCompat.Builder(this)
			.setContentTitle("New Clip")
			.setContentText(clip)
			.setSmallIcon(R.drawable.ic_cloud_clip_light_low)
			.setTicker("New clip!")
			.setAutoCancel(true)
			.setContentIntent(getClipboardPendingIntent())
			.addAction(0, "Call", getCallPendingIntent(clip.toString()))
			.addAction(0, "Web", getWebPendingIntent(clip.toString()))
			.addAction(0, "Directions", getMapPendingIntent(clip.toString()))
			.build()
			;

		notificationManager.notify(mId, notification);
	}

	private PendingIntent getCallPendingIntent(String clip) {		
		String number = clip;
		String uri = "tel:" + number.trim() ;
		Intent intent = new Intent(Intent.ACTION_DIAL);
		intent.setData(Uri.parse(uri));
		
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		return pendingIntent;
	}

	private PendingIntent getMapPendingIntent(String clip) {
		// This just shows the address on the map
		//String address = clip.replace(' ', '+');
		//String uri = "geo:0,0?q=" + address;
		
		// Shows directions to destination from current one
		String destination = clip;
		String uri = "http://maps.google.com/maps?daddr=" + destination;
		Intent intent = new Intent(android.content.Intent.ACTION_VIEW);
		intent.setData(Uri.parse(uri));
		
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		return pendingIntent;
	}
	
	private PendingIntent getWebPendingIntent(String clip) {
		String url = clip;
		Intent intent = new Intent(Intent.ACTION_VIEW);
		intent.setData(Uri.parse(url));
		
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		return pendingIntent;
	}

	private PendingIntent getClipboardPendingIntent() {
		Intent intent = new Intent(this, MainActivity.class);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
		
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		return pendingIntent;
	}
	
	// We need Android 4.1 Jellybean for in-notification actions
	private boolean minimumJellyBean() {
		return android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN;
	}

	private ClipObserver clipObserver = null;;

	private void attachClipObserver() {
		clipObserver = new ClipObserver();
		clipManager.observeClipset(clipObserver);
	}

	private RadioGroup clipboardGroup;
	private RadioButton selectedClip;

	private Button deleteButton;
	private Button copyToSysButton;
	private Button copyToWirelessButton;
	
	public void addButtonListeners() {

		clipboardGroup = (RadioGroup) findViewById(R.id.radioClips);

		deleteButton = (Button) findViewById(R.id.deleteClipButton);
		copyToSysButton = (Button) findViewById(R.id.sendToClipButton);
		copyToWirelessButton = (Button) findViewById(R.id.sendToWirelessButton);		
		
		/*
		 * Delete Clip Button
		 */
		deleteButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {

				int selectedId = clipboardGroup.getCheckedRadioButtonId();
				selectedClip = (RadioButton) findViewById(selectedId);	
				
				if (selectedClip != null) {
					ClipData clip = (ClipData) selectedClip.getTag();
					clipManager.dispose(clip);
					if (clipPersistence.containsKey(clip)) {
						clipPersistence.get(clip).delete();
						clipPersistence.remove(clip);
					}
					Toast.makeText(MainActivity.this, "Deleted", Toast.LENGTH_SHORT).show();
				} else {
					Toast.makeText(MainActivity.this, "No clip selected", Toast.LENGTH_SHORT).show();
				}
			}
		});

		/*
		 * Send clip to system clipboard
		 */
		copyToSysButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {

				int selectedId = clipboardGroup.getCheckedRadioButtonId();
				selectedClip = (RadioButton) findViewById(selectedId);

				if (selectedClip != null) {
					clipManager.sendToPrimaryClipboard((ClipData) selectedClip.getTag());
					Toast.makeText(MainActivity.this, "Sent to device's clipboard",	Toast.LENGTH_SHORT).show();
				} else { 
					Toast.makeText(MainActivity.this, "No clip selected", Toast.LENGTH_SHORT).show();
				}	
			}
		});

		/*
		 * Send clip to wireless clipboard
		 */
		copyToWirelessButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {

				int selectedId = clipboardGroup.getCheckedRadioButtonId();
				selectedClip = (RadioButton) findViewById(selectedId);
				
				if (selectedClip != null) {
					clipManager.sendToWirelessClipboard((ClipData) selectedClip.getTag());
					Toast.makeText(MainActivity.this, "Sent to wireless clipboard", Toast.LENGTH_SHORT).show();
				} else {
					Toast.makeText(MainActivity.this, "No clip selected", Toast.LENGTH_SHORT).show();

				}
			}
		});
	}
}
