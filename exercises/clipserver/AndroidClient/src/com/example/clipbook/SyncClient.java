package com.example.clipbook;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class SyncClient {
	private final static String TAG = SyncClient.class.getName();
	
	/*
	 * Informs SyncClient users when the client has connected to SyncService
	 */
	public interface ClientConnection {
		void onClientConnected();
		void onClientDisconnected();
	}
	
	private Context context = null;
	private ICcdiAidlRpc binder = null;
	private ClientConnection notifier = null;
	private final ServiceConnection connection = new ServiceConnection () {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            binder = ICcdiAidlRpc.Stub.asInterface(service);
            if (notifier != null) {
            	notifier.onClientConnected();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        	if (binder != null) {
        		binder = null;
        	}
        	if (notifier != null) {
        		notifier.onClientDisconnected();
        	}
            Log.e(TAG, "lost connection to sync service!");
        }
	};
	
    /*
     * This is the interface to the Sync client to be used remotely over IPC.
     * The generated API serializes to byte stream which is sent to AIDL.  
     */
	private SyncServiceRemoteClient client = null;
    private final class SyncServiceRemoteClient extends CCDIServiceClient {
    	public SyncServiceRemoteClient() {
    		super(new AbstractByteArrayProtoChannel() {
    			@Override
                protected byte[] perform(byte[] serializedRequest) throws RemoteException {
    				return binder.protoRpc(serializedRequest);
                }}, 
                true);
    	}
    }
    
    /*
     * Life-cycle management API
     */
	public SyncClient(Context context, ClientConnection notifier) {
		this.context = context;
		this.notifier = notifier;
		this.client = new SyncServiceRemoteClient();
        boolean result = context.bindService(new Intent(Constant.ACTION_SYNC_SERVICE), connection, Context.BIND_AUTO_CREATE);
        if (!result) {
            Log.e(TAG, "Unable bind to " + Constant.ACTION_SYNC_SERVICE);
        }
	}
	
	public void releaseService() {
		context.unbindService(connection);
	}

	@Override
	protected void finalize() throws Throwable {
		releaseService();
		super.finalize();
	}

	public boolean hasService() {
		return binder != null;
	}
	
	/*
	 * Returns error codes from client operations executed asynchronously
	 */
	public static class ClientException extends Exception {
		private static final long serialVersionUID = 8523477763322004498L;
		
		private int errorCode = 0;
		public ClientException(int errorCode) {
			this.errorCode = errorCode;
		}
		public int getErrorCode() {
			return errorCode;
		}
	}
	
	/*
	 * The current context state of the sync daemon
	 */
    public static final class SystemState {
    	private long userId = 0;
    	private long deviceId = 0;
    	private PowerMode_t powerMode = PowerMode_t.POWER_NO_SYNC;
    	
    	public void setUserId(long userId) {
    		this.userId = userId;
    	}
    	public long getUserId() {
    		return userId;
    	}
    	
    	public void setDeviceId(long deviceId) {
    		this.deviceId = deviceId;
    	}
    	public long getDeviceId() {
    		return deviceId;
    	}
    	
    	public void setPowerMode(PowerMode_t powerMode) {
    		this.powerMode = powerMode;
    	}
    	public PowerMode_t getPowerMode() {
    		return powerMode;
    	}
    }
    
    public enum SystemType {
    	NONE,
    	USER,
    	DEVICE,
    	POWER,
    	NETWORK
    }
    
    /*
     * Enable sync feature and sync path
     */
    public static final class FeatureSpec {
    	private String path = null;
    	private FeatureType type = FeatureType.NONE;
    	
    	public FeatureSpec (FeatureType type, String path) {
    		this.type = type;
    		this.path = path;
    	}
    	
    	public FeatureType getType() {
    		return type;
    	}
    	
    	public String getPath() {
    		return path;
    	}
    }
    
    public enum FeatureType {
    	NONE,
    	NOTES
    }
	/*
	 * Event Queue is the asynchronous communication from CCD back to the application. 
	 * One should be created (and listened on) by every client.
	 */
	private volatile EventQueue eventQueue = null; // accessed from executor, dispatch, and/or UI thread
	
	public interface EventListener {
		void onEvent(CcdiEvent event);
	}
	
	public static final class EventQueue {
		private long handle = 0;
		private ArrayList<EventListener> listeners = new ArrayList<EventListener>();
		public int listen(EventListener listener) {
			int index = -1;
			if (listener != null) {
				listeners.add(listener);
				index += listeners.size();
			}
			return index;
		}
		public void removeListener(int index) {
			listeners.remove(index);
		}
		public void clearListners() {
			listeners.clear();
		}
		
		// ----- Used exclusively by SyncClient (as parent factory)
		
		EventQueue (long handle) {
			this.handle = handle;
		}
		long getHandle() {
			return handle;
		}
		void dispatch(List<CcdiEvent> source) {
			for (CcdiEvent event : source) {
				for (EventListener listener : listeners) {
					listener.onEvent(event);
				}
			}
		}
	}
    
	/*
	 * Allows us to execute tasks outside the UI thread (with simple future-based result API)
	 */
	private final Executor executor = Executors.newSingleThreadExecutor();
	
	/*
	 * Implement client tasks in a callable form (to be packaged in runnable futures)
	 */
	
	private final class CreateEventQueueTask implements Callable<EventQueue> {
		private EventListener initialListener = null;
		public CreateEventQueueTask(EventListener initialListener) {
			this.initialListener = initialListener;
		}
		@Override
		public EventQueue call() throws Exception {
	    	if (eventQueue == null) {
		    	Log.i(TAG, "starting create event queue");
		    	
	    		EventsCreateQueueOutput.Builder responseBuilder = EventsCreateQueueOutput.newBuilder();
	    		EventsCreateQueueInput.Builder requestBuilder = EventsCreateQueueInput.newBuilder();

	    		int result = client.EventsCreateQueue(requestBuilder.build(), responseBuilder);
	    		if (result < 0) {
	    			throw new ClientException(result);
	    		}

	    		eventQueue = new EventQueue(responseBuilder.getQueueHandle());
	    		eventQueue.listen(initialListener);
	    		eventQueueDispatchThread.start();
	    	}
	    	
	        return eventQueue;
		}
	}
	
	private final class DestroyEventQueueTask implements Callable<Void> {
		@Override
		public Void call() throws Exception {
	    	if (eventQueue != null) {
		    	Log.i(TAG, "starting destroy event queue for " + eventQueue.handle);
		    	
	    		NoParamResponse.Builder responseBuilder = NoParamResponse.newBuilder();
	    		EventsDestroyQueueInput.Builder requestBuilder = EventsDestroyQueueInput.newBuilder()
	    				.setQueueHandle(eventQueue.getHandle());

	    		int result = client.EventsDestroyQueue(requestBuilder.build(), responseBuilder);
	    		if (result < 0) {
	    			throw new ClientException(result);
	    		}
	    		
	    		eventQueue = null; 
	    		eventQueueDispatchThread.interrupt();
	    	}
	        
			return null;
		}
	}
	
	private final Thread eventQueueDispatchThread = new Thread () {
		@Override
		public void run() {
			super.run();
			while (!interrupted())
			{
				EventsDequeueOutput.Builder responseBuilder = EventsDequeueOutput.newBuilder();
				EventsDequeueInput.Builder requestBuilder = EventsDequeueInput.newBuilder()
						.setQueueHandle(eventQueue.getHandle())
						.setMaxCount(5)			// max number of events to dequeue
						.setTimeout(180000);	// timeout while waiting for next event

				try {
					int result = client.EventsDequeue(requestBuilder.build(), responseBuilder);
					if (0 > result) {
						Log.e(TAG, "event dequeue error code: " + result);
					}
				} catch (ProtoRpcException e) {
					Log.e(TAG, "rpc exception: " + e.getMessage());
				}

				eventQueue.dispatch(responseBuilder.getEventsList());
			}
		}
	};
	
	private final class LoginTask implements Callable<Boolean> {
		private String username;
		private String password;
		
		public LoginTask(String username, String password) {
			this.username = username;
			this.password = password;
		}
		
		@Override
		public Boolean call() throws Exception {
	    	Log.i(TAG, "starting login for " + username);
	    	
	        LoginOutput.Builder responseBuilder = LoginOutput.newBuilder();
	    	LoginInput.Builder requestBuilder = LoginInput.newBuilder()
	    			.setUserName(username)
	            	.setPassword(password);

	    	int result = client.Login(requestBuilder.build(), responseBuilder);
	        return result == 0;
		}
	}
	
	private final class QuerySystemStateTask implements Callable<SystemState> {
		EnumSet<SystemType> systems = null;
		public QuerySystemStateTask(EnumSet<SystemType> systems) {
			this.systems = systems;
		}
		@Override
		public SystemState call() throws Exception {
	    	Log.i(TAG, "starting get system state");
	    	
	    	GetSystemStateOutput.Builder responseBuilder = GetSystemStateOutput.newBuilder();
	    	GetSystemStateInput.Builder requestBuilder = GetSystemStateInput.newBuilder()
	    			.setGetPlayers(systems.contains(SystemType.USER))
	    			.setGetDeviceId(systems.contains(SystemType.DEVICE))
	    			.setGetPowerMode(systems.contains(SystemType.POWER))
	    			.setGetNetworkInfo(systems.contains(SystemType.NETWORK));
	    	
	    	int result = client.GetSystemState(requestBuilder.build(), responseBuilder);
    		if (result < 0) {
    			throw new ClientException(result);
    		}
    		
    		SystemState state = new SystemState();
    		GetSystemStateOutput output = responseBuilder.build();
    		
    		state.setDeviceId(output.getDeviceId());
    		state.setPowerMode(output.getPowerModeStatus().getPowerMode());
    		if (output.getPlayers().getPlayersCount() > 0) {
    			state.setUserId(output.getPlayers().getPlayers(0).getUserId());
    		}

    		return state;
		}
	}
	
	private final class EnableFeatureTask implements Callable<Boolean> {
		private SystemState state = null;
		private FeatureSpec feature = null;
		public EnableFeatureTask(SystemState state, FeatureSpec feature) {
			this.state = state;
			this.feature = feature;
		}
		@Override
		public Boolean call() throws Exception {
	    	Log.i(TAG, "enabling feature " + feature.getType());
	    	
    		int result = -1;
	    	if (state != null) {
	    		boolean hasFeature = false;
	    		UpdateSyncSettingsOutput.Builder responseBuilder = UpdateSyncSettingsOutput.newBuilder();
	    		UpdateSyncSettingsInput.Builder requestBuilder = UpdateSyncSettingsInput.newBuilder()
	    				.setUserId(state.getUserId());
	    		SyncFeatureSettingsRequest.Builder settingsBuilder = SyncFeatureSettingsRequest.newBuilder()
	    				.setEnableSyncFeature(true)
	    				.setSetSyncFeaturePath(feature.getPath());

	    		switch (feature.getType()) {
	    		case NOTES:
	    			requestBuilder.setConfigureNotesSync(settingsBuilder.build());
	    			hasFeature = true;
	    			break;

	    		default:
	    			Log.e(TAG, "cannot enable unknown feature type " + feature.getType());
	    			break;
	    		}

	    		if (hasFeature) {
	    			try {
	    				result = client.UpdateSyncSettings(requestBuilder.build(), responseBuilder);
	    			} catch (AppLayerException e) {
	    				Log.e(TAG, "dataset may not be created for the user");
	    			}
	    		}
	    	}
	    		
			return result == 0;
		}
	}
	
	/*
	 * Public functional API
	 */
    
    public AsyncResult<EventQueue> createEventQueue(EventListener listener) {
    	CreateEventQueueTask task = new CreateEventQueueTask(listener);
    	AsyncResult<EventQueue> future = new AsyncResult<EventQueue>(task);
    	executor.execute(future);
    	return future;
    }
    
    public AsyncResult<Void> destroyEventQueue() {
    	DestroyEventQueueTask task = new DestroyEventQueueTask();
    	AsyncResult<Void> future = new AsyncResult<Void>(task);
    	executor.execute(future);
    	return future;
    }
    
    public AsyncResult<Boolean> login(String username, String password) {
    	LoginTask task = new LoginTask(username, password);
    	AsyncResult<Boolean> future = new AsyncResult<Boolean>(task);
    	executor.execute(future);
    	return future;
    }
	
    private SystemState currentState = null;
	
    public AsyncResult<SystemState> querySystemState(EnumSet<SystemType> systems) {
    	QuerySystemStateTask task = new QuerySystemStateTask(systems);
    	AsyncResult<SystemState> future = new AsyncResult<SystemState>(task);
    	future.whenDone(new AsyncResult.Notifier<SystemState>() {
			@Override
			public void observe(AsyncResult<SystemState> result) {
				try {
					currentState = result.get(); // always keep a local copy
				} catch (Exception e) {
					Log.w(TAG, "sync client unable to get current state");
					currentState = null;
				}
			}	
    	});
    	executor.execute(future);
    	return future;
    }
    
    public AsyncResult<Boolean> enableFeature(FeatureSpec feature) {
    	EnableFeatureTask task = new EnableFeatureTask(currentState, feature);
    	AsyncResult<Boolean> future = new AsyncResult<Boolean>(task);
    	executor.execute(future);
    	return future;
    }

	public AsyncResult<Integer> linkDevice(int userID, String deviceName) {
		LinkDeviceTask task = new LinkDeviceTask(userID, deviceName);
    	AsyncResult<Integer> future = new AsyncResult<Integer>(task);
    	executor.execute(future);
    	return future;
	}
	
	private final class LinkDeviceTask implements Callable<Integer> {
		
		private int userID;
		private String deviceName;
		
		public LinkDeviceTask(int userID, String deviceName) {
			this.userID = userID;
			this.deviceName = deviceName;
		}
		
		@Override
		public Integer call() throws Exception {
	    	Log.i(TAG, "starting to link device");
	    	
	    	LinkDeviceInput request = LinkDeviceInput.newBuilder().setUserId(userID).setDeviceName(deviceName).build();
            NoParamResponse.Builder responseBuilder = NoParamResponse.newBuilder();
	    	
	    	int result = client.LinkDevice(request, responseBuilder);
	        return result;
		}
	}
}
