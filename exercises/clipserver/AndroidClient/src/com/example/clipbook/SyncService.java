package com.example.clipbook;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;

public class SyncService extends Service {
	private final static String TAG = SyncService.class.getName();
	
	private static SyncDaemon daemon = new SyncDaemon();
	private final SyncServiceRemoteBinder binder = new SyncServiceRemoteBinder(); 
	private final SyncServiceLocalClient client = new SyncServiceLocalClient();
	
	@Override
	public void onCreate() {
		super.onCreate();
		new Thread() { // start daemon on its own thread
			@Override
			public void run () {
				daemon.start(getApplicationContext().getFilesDir().getAbsolutePath());
				while (!daemon.hasStarted()) {
					try {
						Thread.sleep(25);
					} catch (InterruptedException e) {} // not expecting any interruption
				}
			}
		}.start();
	}

	@Override
	public void onDestroy() {
		daemon.stop();
		super.onDestroy();
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		// on startService, keep the service running until explicitly shut down
		return START_STICKY;
	}

	@Override
	public IBinder onBind(Intent intent) {
		return (IBinder) binder;
	}
	
	/*
	 * This binder uses Aidl to allow serialized messages to be sent as byte
	 * streams across process boundaries; the bytes are then directly routed
	 * to the daemon process where they are deserialzed and run. This binder
	 * does not provide a convenient method-based API, and should be combined
	 * with CCDIServiceClient for serialization from method calls.
	 */
    public class SyncServiceRemoteBinder extends ICcdiAidlRpc.Stub {
        @Override
        public byte[] protoRpc(byte[] serializedRequest) {
            return daemon.remoteRequest(serializedRequest);
        }

		@Override
		public boolean isReady() throws RemoteException {
			return daemon.hasStarted();
		}
    }
    
    /*
     * This is the interface to the Sync client to be used locally in the Service 
     * process. The generated API always serialized to byte stream, but this version
     * simply routes the bytes to daemon directly. The "remote" version uses IPC.
     */
    private class SyncServiceLocalClient extends CCDIServiceClient {
    	public SyncServiceLocalClient() {
    		super(new AbstractByteArrayProtoChannel() {
    			@Override
                protected byte[] perform(byte[] serializedRequest) {
                    return daemon.localRequest(serializedRequest);
                }}, 
                true);
    	}
    }
}
