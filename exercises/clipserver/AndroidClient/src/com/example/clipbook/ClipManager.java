package com.example.clipbook;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import com.example.clipbook.MessageManager.ContentNotification;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;

public class ClipManager {
	private static final String TAG = ClipManager.class.getName();
	
	private SharedPreferences preferences = null;
	private ScheduledExecutorService executor = null;

	private ClipboardListener clipboardListener = new ClipboardListener();
	private class ClipboardListener implements ClipboardManager.OnPrimaryClipChangedListener {
	    private Future<?> clipboardListenerTask = null;
		public void start() {
			// HACK: clipboard notifications are currently broken in android
			clipboardListenerTask = executor.scheduleAtFixedRate(new Runnable() {
				private ClipData lastClip = null;
				public void run() {
					try {
						if (clipboard.hasPrimaryClip()) {
							ClipData currClip = clipboard.getPrimaryClip();
							// HACK: Google neglected to overload equals for any of its clipboard classes (nor CharSequence impl)
							String currClipText = currClip.getItemAt(0).coerceToText(context).toString();
							String lastClipText = (lastClip != null)? lastClip.getItemAt(0).coerceToText(context).toString() : null;
							
							// Check all clips in clipManager
							if (!bufferContainsClip(currClipText) && !currClipText.equals(lastClipText)) {
								onPrimaryClipChanged();
								lastClip = currClip;								
							}
						}
					} catch (Exception e) {
						Log.e(TAG, "Error found in clipboard listener.");
						e.printStackTrace();
					}
				}
			}, 0, 1, TimeUnit.SECONDS);
		}
		
		public void stop() {
			clipboardListenerTask.cancel(true);
		}

		@Override
		public void onPrimaryClipChanged() {
			ClipManager.this.notify(clipboard.getPrimaryClip());			
		}
	}

	private final Context context;
	private final ClipboardManager clipboard;
	private final MessageManager messageManager = new MessageManager();
	private volatile boolean reconnecting = false;
	private final int MAX_CLIPS;
	
	private final static int CLIP_BUFFER_CHANGED = 1013;
	private final static int CLIP_MANAGER_RESTART = 1014;
	private final static Handler notifyHandler = new Handler(Looper.getMainLooper()) {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case CLIP_BUFFER_CHANGED:
				removeMessages(CLIP_BUFFER_CHANGED);
				((ClipManager) msg.obj).notifyObservers();
				break;
			case CLIP_MANAGER_RESTART:
				removeMessages(CLIP_MANAGER_RESTART);
				((ClipManager) msg.obj).restart();
			default:
				super.handleMessage(msg);
			}
		}
	};
	
	public ClipManager(Context context, int maxClips) {
		this.context = context;
		this.clipboard = (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
		this.preferences = PreferenceManager.getDefaultSharedPreferences(context);
		this.MAX_CLIPS = maxClips;
		
		this.messageManager.onContentAvailable(new Observer<MessageManager.ContentNotification>() {
			@Override
			public void observe(ContentNotification remoteContent) {
				ClipManager.this.notify(remoteContent);
			}
		});
		
		this.messageManager.onDisconnect(new Observer<Exception>() {
			@Override
			public void observe(Exception reason) {
				if (reconnecting == false) {
					reconnecting = true;
					try {
						Log.i(TAG, "reconnecting due to exception");
						reason.printStackTrace();
						executor.schedule(new Runnable() {
							@Override
							public void run() {
								notifyHandler.sendMessage(notifyHandler.obtainMessage(CLIP_MANAGER_RESTART, ClipManager.this));
							}
						}, 10, TimeUnit.SECONDS);
					} catch (RejectedExecutionException e) {
						reconnecting = false;
					}
				}
			}
		});
	}
	
	public void start() {
		if (executor == null) {
			executor = Executors.newSingleThreadScheduledExecutor();
		}
		
		String address = context.getResources().getString(R.string.server_address);
		int port = Integer.parseInt(context.getResources().getString(R.string.server_port));
		long userId = Util.generateHash63(context.getString(R.string.account_name));
		long deviceId = Util.generateHash63(context.getString(R.string.device_name));
		long secret = Util.generateHash63(context.getString(R.string.account_secret));
		
		String preferredAccountName = preferences.getString("account_name", "");
		if (preferredAccountName.length() > 0) {
			Log.i(TAG, "found preferred account name: " + preferredAccountName);
			userId = Util.generateHash63(preferredAccountName);
		}

		String preferredDeviceName = preferences.getString("device_name", "");
		if (preferredDeviceName.length() > 0) {
			Log.i(TAG, "found preferred device name: " + preferredDeviceName);
			deviceId = Util.generateHash63(preferredDeviceName);
		}

		if (messageManager.isConnected()) {
			messageManager.disconnect();
		}

		AsyncResult<Boolean> result = messageManager.connect(address, port);	
		try {
			if (result.get()) {
				result = messageManager.authenticate(userId, deviceId, secret);			
				Log.i(TAG, "start authenticating");
				result.whenDone(new Observer<AsyncResult<Boolean>>() {
					@Override
					public void observe(AsyncResult<Boolean> result) {
						Log.i(TAG, "done authenticating");
						try {
							if (result.get()) {
								clipboardListener.start();
							} else {
								Log.i(TAG, "failed to authenticate");
							}
						} catch (Exception e) {
							Log.i(TAG, "exception while authenticating");
							e.printStackTrace();
						} finally {
							reconnecting = false;
						}
					}
				});
			} else {
				Log.i(TAG, "failed to connect");
				reconnecting = false;
			}
		} catch (Exception e) {
			Log.i(TAG, "exception while connecting");
			e.printStackTrace();
			reconnecting = false;
		}
	}
	
	public void restart() {
		stop();
		start();
	}
	
	public void stop() {
		clipboardListener.stop();
	}
	
	public void wake() {
		messageManager.wake();
	}
	
	public void sleep() {
		messageManager.sleep();
	}
	
	public void dispose() {
		clipboardListener.stop();
		messageManager.dispose();
		executor.shutdownNow();
	}

	private boolean bufferContainsClip(String newClipText) {
		
		for (ClipData clip : clipBuffer) {
			String lastClipText = clip.getItemAt(0).coerceToText(context).toString();
			if (lastClipText.equals(newClipText)) {
				return true;
			}
		}
		return false;
	}
	
	private List<Observer<List<ClipData>>> clipsetObservers = new ArrayList<Observer<List<ClipData>>>();
	private List<ClipData> clipBuffer = new ArrayList<ClipData>();

	protected void notifyObservers() {
		for (Observer<List<ClipData>> observer : clipsetObservers) {
			observer.observe(clipBuffer);
		}
	}
	
	public void observeClipset(Observer<List<ClipData>> observer) {
		if (observer != null) {
			clipsetObservers.add(observer);
		}
	}
	
	public void notify(ClipData clip) {
		Log.i(TAG, "------------- notified of clip " + clip);

		clipBuffer.add(clip);
		if (clipBuffer.size() > MAX_CLIPS) {
			dispose(clipBuffer.get(0)); // remove oldest (by insert order)
		}
		
		notifyHandler.sendMessage(notifyHandler.obtainMessage(CLIP_BUFFER_CHANGED, this));
	}
	
	private final Set<String> remoteContentCache = new TreeSet<String>();
	
	public void notify(ContentNotification remoteContent) {
		String description = remoteContent.getDescription();
		List<String> contentType = remoteContent.getContentType();
		String[] mimeTypes = contentType.toArray(new String[contentType.size()]);
		
		synchronized (clipBuffer) {
			remoteContentCache.add(description); // HACK: should instead give a proper notification to outside class
			ClipData.Item item = new ClipData.Item(description);
			clipBuffer.add(new ClipData(description, mimeTypes, item));
			if (clipBuffer.size() > MAX_CLIPS) {
				dispose(clipBuffer.get(0)); // remove oldest (by insert order)
			}
		}
		
		notifyHandler.sendMessage(notifyHandler.obtainMessage(CLIP_BUFFER_CHANGED, this));
	}
	
	public void dispose(ClipData clip) {
		clipBuffer.remove(clip);
		
		notifyHandler.sendMessage(notifyHandler.obtainMessage(CLIP_BUFFER_CHANGED, this));
	}
	
	public void sendToPrimaryClipboard(ClipData clip) {
		clipboard.setPrimaryClip(clip);
	}
	
	public void sendToWirelessClipboard(ClipData clip) {

		for (int i=0; i < clip.getDescription().getMimeTypeCount(); ++i)
			Log.i(TAG, "mimeType: " + clip.getDescription().getMimeType(i));
		
		Long contentId = Util.generateRand63();
		List<String> contentType = Arrays.asList(clip.getDescription().filterMimeTypes("*/*"));
		String description = clip.getItemAt(0).coerceToText(context).toString();
		
		if (remoteContentCache.contains(description) == false) {
			messageManager.notify(contentId, contentType, description);
		}
	}
}
