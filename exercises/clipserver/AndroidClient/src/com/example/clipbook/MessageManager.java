package com.example.clipbook;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.util.ArrayDeque;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.Queue;
import java.util.TreeMap;
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

public class MessageManager {
	private static final String TAG = MessageManager.class.getName();
	
	public class ContentNotification {
		private final Long contentId;
		private final List<String> contentType;
		private final String description;
		public ContentNotification (Long contentId, List<String> contentType, String description) {
			this.contentId = contentId;
			this.contentType = contentType;
			this.description = description;
		}
		public Long getContentId() {
			return contentId;
		}
		public List<String> getContentType() {
			return contentType;
		}
		public String getDescription() {
			return description;
		}
	}
	
	private final int BUFFER_SIZE = 2048;
	private volatile ByteBuffer buffer = ByteBuffer.allocateDirect(BUFFER_SIZE);
	private volatile SocketChannel channel = null;
	private InetSocketAddress endpoint = null;
	
	private volatile long token;
	
	private final Map<Integer, List<Observer<JSONObject>>> handlers = new TreeMap<Integer, List<Observer<JSONObject>>>();
	private final Map<Integer, Queue<Observer<JSONObject>>> rpchandlers = new TreeMap<Integer, Queue<Observer<JSONObject>>>();
	private final Map<Long, String> contentCache = new TreeMap<Long,String>();
	
    private enum MessageType
    {
        HEADER(100),
        AUTHENTICATE_REQUEST(101),
        AUTHENTICATE_RESPONSE(102),
        REFRESH_INDEX_REQUEST(103),
        REFRESH_INDEX_RESPONSE(104),
        NOTIFY_AVAILABLE_REQUEST(105),
        SYNC_REQUEST(106),
        SYNC_RESPONSE(107),
        PING(108);
        
        private final int value;
		private MessageType(int value) {
        	this.value = value;
        }
		public int getValue() {
			return value;
		}
    }
    
    private enum DatasetType
    {
        NONE(0),
        CLIPBOARD(1),
        NUM_DATASETS(2);
        
        private final int value;
		private DatasetType(int value) {
        	this.value = value;
        }
		public int getValue() {
			return value;
		}
    }
    
	private ScheduledExecutorService executor = null;
    private volatile boolean receiveThreadContinue = true;
    private Thread receiveThread = null;
    private Future<?> heartbeatTask = null;
    
    public MessageManager() {
    	rpchandlers.put(MessageType.AUTHENTICATE_REQUEST.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	rpchandlers.put(MessageType.AUTHENTICATE_RESPONSE.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	rpchandlers.put(MessageType.REFRESH_INDEX_REQUEST.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	rpchandlers.put(MessageType.REFRESH_INDEX_RESPONSE.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	rpchandlers.put(MessageType.NOTIFY_AVAILABLE_REQUEST.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	rpchandlers.put(MessageType.SYNC_REQUEST.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	rpchandlers.put(MessageType.SYNC_RESPONSE.getValue(), new ArrayDeque<Observer<JSONObject>>());
    	
    	handlers.put(MessageType.AUTHENTICATE_REQUEST.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.AUTHENTICATE_RESPONSE.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.REFRESH_INDEX_REQUEST.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.REFRESH_INDEX_RESPONSE.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.NOTIFY_AVAILABLE_REQUEST.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.SYNC_REQUEST.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.SYNC_RESPONSE.getValue(), new ArrayList<Observer<JSONObject>>());
    	handlers.put(MessageType.PING.getValue(), new ArrayList<Observer<JSONObject>>());
    	
    	handlers.get(MessageType.NOTIFY_AVAILABLE_REQUEST.getValue()).add(new Observer<JSONObject>() {
			@Override
			public void observe(JSONObject object) {
				Long contentId;
				try {
					contentId = object.getLong("content_id");
					
					List<String> contentType = new ArrayList<String>();
					JSONArray array = object.getJSONArray("content_type");
					for (int i=0; i < array.length(); ++i) {
						contentType.add(array.getString(i));
					}
					
					String description = object.getString("description");

					notifyContentAvailable(new ContentNotification(contentId, contentType, description));
					
				} catch (JSONException e) {
					e.printStackTrace();
				}
			}
		});
    	
    	handlers.get(MessageType.SYNC_REQUEST.getValue()).add(new Observer<JSONObject>() {
			@Override
			public void observe(JSONObject object) {
				Long contentId;
				try {
					contentId = object.getLong("content_id");
					String content = null;
					int message = 0;
					
					synchronized (contentCache) {
						if (contentCache.containsKey(contentId)) {
							content = contentCache.get(contentId);
							message = 1;
						}
					}
					try {
						executor.execute(new AsyncResult<Void>(new SyncResponseTask(message, contentId, content)));
					}
					catch (RejectedExecutionException e) {
						Log.i(TAG, "rejected: " + e.getMessage());
					}
					
				} catch (JSONException e) {
					e.printStackTrace();
				}
			}
		});
    	
    	disconnectObservers.add(new Observer<Exception>() {
			@Override
			public void observe(Exception reason) {
				try {
					channel.close();
				} catch (IOException e) {
					Log.i(TAG, "unable to close channel");
					e.printStackTrace();
				}
			}
		});
    }
    
    private List<Observer<Exception>> disconnectObservers = new ArrayList<Observer<Exception>>();
    private void notifyDisconnect(Exception reason) {
    	synchronized (disconnectObservers) {
    		for (Observer<Exception> observer : disconnectObservers) {
    			observer.observe(reason);
    		}
		}
    }
    public void onDisconnect(Observer<Exception> observer) {
    	synchronized (contentObservers) {
    		disconnectObservers.add(observer);
    	}
    }
    
    private List<Observer<ContentNotification>> contentObservers = new ArrayList<Observer<ContentNotification>>();
    private void notifyContentAvailable(ContentNotification contentNotification) {
    	synchronized (contentObservers) {
    		for (Observer<ContentNotification> observer : contentObservers) {
    			observer.observe(contentNotification);
    		}
		}
    }
    public void onContentAvailable(Observer<ContentNotification> observer) {
    	synchronized (contentObservers) {
    		contentObservers.add(observer);
    	}
    }
    
    private boolean send(JSONObject serialization) throws IOException {
    	boolean proceed = true;
    	byte[] bytes = serialization.toString().getBytes();
    	ByteBuffer message = ByteBuffer.wrap(bytes);
    	ByteBuffer header = ByteBuffer.allocate(2);
    	header.putShort((short) bytes.length); // default byte buffer order is always BIG_ENDIAN
    	header.flip();

    	proceed = proceed && channel.write(header) == 2;
    	proceed = proceed && channel.write(message) == bytes.length;
		
		return proceed;
    }
    
    private boolean send(byte[] bytes) throws IOException {
    	boolean proceed = true;
    	ByteBuffer message = ByteBuffer.wrap(bytes);
    	proceed = channel.write(message) == bytes.length;
		
		return proceed;
    }
    
    private void receive() {
		Exception reason = null;
		boolean proceed = receiveThreadContinue;
		while (proceed) {
			try {
				proceed = receiveThreadContinue && channel.isConnected();
				if (proceed)
				{
					ByteBuffer headerbuf = ByteBuffer.wrap(buffer.array(), 0, 2);
					long headerRead = channel.read(headerbuf);
					
					headerbuf.flip();
					short payloadSize = headerbuf.getShort();
					Log.i(TAG, "read header: " + headerRead);
					Log.i(TAG, "reading payload: " + payloadSize);

					ByteBuffer payloadbuf = ByteBuffer.wrap(buffer.array(), 0, payloadSize);
					long messageRead = channel.read(payloadbuf);
					Log.i(TAG, "read message: " + messageRead);

					JSONObject serialization = new JSONObject(new String(payloadbuf.array()));
					int message_id = serialization.getInt("message_id");
					Log.i(TAG, "message id: " + message_id);

					synchronized (handlers) {
						if (handlers.containsKey(message_id)) {
							Log.i(TAG, "got handler for message_id " + message_id);
							for (Observer<JSONObject> observer : handlers.get(message_id)) {
								Log.i(TAG, "calling observer");
								observer.observe(serialization);
							}
						}
					}
					
					synchronized (rpchandlers) {
						if (rpchandlers.containsKey(message_id)) {
							Log.i(TAG, "got rpchandler for message_id " + message_id);
							Queue<Observer<JSONObject>> handlers = rpchandlers.get(message_id);
							if (handlers.size() > 0) {
								Observer<JSONObject> observer = handlers.remove();
								observer.observe(serialization);
							}
						}
					}
				}
			} catch (Exception e) {
				Log.i(TAG, "receive failed, disconnecting...");
				proceed = false;
				reason = e;
			}
		}
		notifyDisconnect(reason);
    }
	
	private final class ConnectTask implements Callable<Boolean> {
		private SocketAddress address = null;
		public ConnectTask(SocketAddress address) {
			this.address = address;
		}
		@Override
		public Boolean call() throws Exception {
			boolean connected = channel != null && channel.isConnected(); 
			try {
				if (!connected)
				{
					Log.i(TAG, "address: " + address);
					channel = SocketChannel.open();
					channel.connect(address);
					while ((connected = channel.finishConnect()) == false);
				}
			} catch (Exception e) {
				notifyDisconnect(e);
			}
			return connected;
		}
	}
	
	private final class AuthenticateTask implements Callable<Boolean> {
		private Long userId;
		private Long deviceId;
		private Long secret;
		public AuthenticateTask(Long userId, Long deviceId, Long secret) {
			this.userId = userId;
			this.deviceId = deviceId;
			this.secret = secret;
		}
		@Override
		public Boolean call() throws Exception {
			Boolean proceed;
			try {
				JSONObject serialization = new JSONObject();
				serialization.put("message_id", MessageType.AUTHENTICATE_REQUEST.getValue());
				serialization.put("user_id", userId);
				serialization.put("device_id", deviceId);
				serialization.put("secret", secret);
				Log.i(TAG, "Authenticating: " + serialization);
				
				token = 0; // forget previous token
				final CountDownLatch latch = new CountDownLatch(1); // synchronize when token is ready
				
				synchronized (rpchandlers) {
					rpchandlers.get(MessageType.AUTHENTICATE_RESPONSE.getValue()).add(new Observer<JSONObject>() {
						@Override
						public void observe(JSONObject object) {
							try {
								Log.i(TAG, "received auth reply: " + object);
								token = object.getLong("token");
								latch.countDown();
							} catch (JSONException e) {
								e.printStackTrace();
							}
						}
					});
				}

				proceed = send(serialization); // send request

				if (proceed) {
					latch.await();
					proceed = token != 0;
				}
				
			} catch (JSONException e) {
				e.printStackTrace();
				proceed = false;
			} catch (Exception e) {
				proceed = false;
				notifyDisconnect(e);
			}

			return proceed;
		}
	}
	
	private final class NotifyAvailableTask implements Callable<Boolean> {
		private Long contentId;
		private List<String> contentType = null;
		private String description = null;
		public NotifyAvailableTask(Long contentId, List<String> contentType, String description) {
			this.contentId = contentId;
			this.contentType = contentType;
			this.description = description;
		}
		@Override
		public Boolean call() throws Exception {
			Boolean proceed;
			try {
				JSONObject serialization = new JSONObject();
				serialization.put("message_id", MessageType.NOTIFY_AVAILABLE_REQUEST.getValue());
				serialization.put("token", token);
				serialization.put("dataset_id", DatasetType.CLIPBOARD.getValue());
				serialization.put("content_id", contentId);
				serialization.put("content_type", new JSONArray (contentType));
				serialization.put("description", description);
				Log.i(TAG, "Notifying: " + serialization);
				proceed = send(serialization);
			} catch (JSONException e) {
				e.printStackTrace();
				proceed = false;
			} catch (Exception e) {
				proceed = false;
				notifyDisconnect(e);
			}

			return proceed;
		}
	}
	
	private final class SyncResponseTask implements Callable<Void> {
		private int message = 0;
		private long contentId = 0;
		private int contentSize = 0;
		private byte[] content = null;
		public SyncResponseTask(int message, long contentId, String content) {
			this.message = message;
			this.contentId = contentId;
			this.contentSize = (message > 0)? content.getBytes().length : 0;
			this.content = (message > 0)? content.getBytes() : null;
		}
		@Override
		public Void call() throws Exception {
			try {
				JSONObject serialization = new JSONObject();
				serialization.put("message_id", MessageType.SYNC_RESPONSE.getValue());
				serialization.put("token", token);
				serialization.put("dataset_id", DatasetType.CLIPBOARD.getValue());
				serialization.put("content_id", contentId);
				serialization.put("content_size", contentSize);
				serialization.put("message", message);
				Log.i(TAG, "SyncResponse: " + serialization);
				boolean proceed = send(serialization);
				if (proceed && message > 0) {
					send(content);
				}
			} catch (JSONException e) {
				e.printStackTrace();
			} catch (Exception e) {
				notifyDisconnect(e);
			}
			return null;
		}
	}
	
	public boolean isConnected() {
		return channel != null && channel.isConnected();
	}
	
	public AsyncResult<Boolean> connect(String host, int port) {
		endpoint = new InetSocketAddress(host, port);
		
		if (executor == null) {
			executor = Executors.newSingleThreadScheduledExecutor();
		};
		
    	AsyncResult<Boolean> future = new AsyncResult<Boolean>(new ConnectTask(endpoint));
    	
    	future.whenDone(new Observer<AsyncResult<Boolean>>() {
			@Override
			public void observe(AsyncResult<Boolean> result) {
				try {
					if (result.get()) {
						receiveThreadContinue = true;
						receiveThread = new Thread (new Runnable() {
							@Override
							public void run() {
								receive();
							}
					    });
						receiveThread.start();
					}
				} catch (InterruptedException e) {
					Log.i(TAG, "connect was interrupted!");
				} catch (Exception e) {
					Log.e(TAG, "connect threw: " + e.getMessage());
					e.printStackTrace();
				}
			}
    	});
    	
	try {
		executor.execute(future);
	}
	catch (RejectedExecutionException e) {
		Log.i(TAG, "rejected: " + e.getMessage());
		Log.i(TAG, "shut down: " + executor.isShutdown());
		future.cancel(false);
	}
    	
    	return future;
	}
	
	public AsyncResult<Boolean> authenticate(Long userId, Long deviceId, Long secret) {
    	AsyncResult<Boolean> future = new AsyncResult<Boolean>(new AuthenticateTask(userId, deviceId, secret));
    	executor.execute(future);
    	
    	return future;
	}
	
	public AsyncResult<Boolean> notify(Long contentId, List<String> contentType, String description) {
		synchronized (contentCache) { // save content for future sync requests
			contentCache.put(contentId, description);
		}

    	AsyncResult<Boolean> future = new AsyncResult<Boolean>(new NotifyAvailableTask(contentId, contentType, description));

	try {
		executor.execute(future);
	}
	catch (RejectedExecutionException e) {
		Log.i(TAG, "rejected: " + e.getMessage());
		future.cancel(false);
	}
    	
    	return future;
	}
	
	public Future<?> heartbeat (int delay, int frequency, TimeUnit unit) {
		return executor.scheduleAtFixedRate(new Runnable() {
			@Override
			public void run() {
				JSONObject serialization = new JSONObject();
				Exception reason = null;
				boolean proceed = false;
				try {
					Log.i(TAG, "pinging...");
					serialization.put("message_id", MessageType.PING.getValue());
					serialization.put("token", token);
					proceed = send(serialization);
				} catch (Exception e) {
					proceed = false;
					reason = e;
				}
				if (proceed == false)
					notifyDisconnect(reason);
			}
		}, delay, frequency, unit);
	}
	
	public void wake() {
		Log.i(TAG, "waking.... ");
		if (heartbeatTask != null && !heartbeatTask.isCancelled()) {
			heartbeatTask.cancel(true);
		}
		heartbeatTask = heartbeat(10, 10, TimeUnit.SECONDS);
	}
	
	public void sleep() {
		Log.i(TAG, "sleeping.... ");
		if (heartbeatTask != null) {
			heartbeatTask.cancel(true);
		}
	}
	
	public void disconnect() {
		receiveThreadContinue = false;
		receiveThread.interrupt();
		try {
			channel.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void dispose() {
		executor.shutdownNow();
		disconnect();
	}
}
