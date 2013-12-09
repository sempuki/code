package com.example.clipbook;

import java.util.ArrayList;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

import android.util.Log;

public class AsyncResult<V> extends FutureTask<V> {
	private static final String TAG = AsyncResult.class.getName();
	
	/*
	 * Notifier observes the async result A. 
	 * 
	 * Prefer this interface for simple notifications. 
	 */
	public interface Notifier<A> extends Observer<AsyncResult<A>> {}
	
	/*
	 * Reactor type observes the an async result A, and generates async future B, registering B's reactor in turn afterward.
	 * Since FutureTask.done() is generally done on the thread that executed the task, the observe and apply methods will too.
	 * 
	 * Prefer this interface for chained async computations.
	 */
	public abstract static class Reactor<A,B> implements Function<AsyncResult<A>, AsyncResult<B>>, Notifier<A> {
		public String name;
		private Reactor<B,?> nextReactor = null;
		public Reactor<B,?> getNextReactor() {
			return nextReactor;
		}
		
		public Reactor<A,B> andThen(Reactor<B,?> nextReactor){
			Log.i(TAG, "%%%%%%%%%%%%%%%%%%%%%%%%%%%% " + name + " -> " + nextReactor.name);
			this.nextReactor = nextReactor;
			return this;
		}

		public void observe(AsyncResult<A> object) {
			AsyncResult<B> result = apply(object);
			Reactor<B, ?> react = getNextReactor();
			if (result != null) {
				result.whenDone(react);
			}
		}

		public void initiate(A input) {
			AsyncResult<A> initialResult = new AsyncResult<A>(new NoOp(), input);
			initialResult.run(); // execute no-op and set the result to the input value
			observe(initialResult);
		}
		
		public void initiate() {
			initiate(null);
		}
	}
	
	public AsyncResult(Callable<V> callable) {
		super(callable);
	}
	
	public AsyncResult(Runnable runnable, V result) {
		super(runnable, result);
	}
		
	private ArrayList<Observer<AsyncResult<V>>> doneObservers = new ArrayList<Observer<AsyncResult<V>>>();
	
	public AsyncResult<V> whenDone(Observer<AsyncResult<V>> observer) {
		if (observer != null) {
			doneObservers.add(observer);
			if (isDone()) {
				observer.observe(this);
			}
		}
		return this;
	}

	/* This method is usually called from the same thread as run() */
	@Override
	protected void done() {
		super.done();
		for (Observer<AsyncResult<V>> observer : doneObservers) {
			observer.observe(this);
		}
	}
}
