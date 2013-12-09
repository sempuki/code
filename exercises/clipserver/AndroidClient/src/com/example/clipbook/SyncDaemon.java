package com.example.clipbook;

public class SyncDaemon extends ServiceSingleton {
	private final static String TAG = SyncDaemon.class.getName();
	private boolean started = false;
	
	public boolean start(final String workingDirectory) {
        return startNativeService(workingDirectory);
	}
	
	public boolean stop() {
		return stopNativeService();
	}
	
	public boolean hasStarted() {
		return started;
	}
	
	public byte[] remoteRequest(byte[] serializedRequest) {
		return ccdiJniProtoRpc(serializedRequest, false);
	}
	
	public byte[] localRequest(byte[] serializedRequest) {
		return ccdiJniProtoRpc(serializedRequest, true);
	}
}
