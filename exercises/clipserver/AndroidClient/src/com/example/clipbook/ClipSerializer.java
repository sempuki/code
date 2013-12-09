package com.example.clipbook;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import android.content.ClipData;
import android.content.Context;
import android.os.Parcel;
import android.util.Log;

public class ClipSerializer {
	private static final String TAG = ClipSerializer.class.getName();
	public static final String FILE_SUFFIX = ".clip";
	public static final int READ_BUFFER_SIZE = 4096;

	private Context context = null;
	private File directory = null;
	private ArrayList<File> files = new ArrayList<File>();
	
	public ClipSerializer(Context context, String serializePath) {
		this.context = context;
		directory = new File(serializePath);
		Log.i(TAG, " starting file serialization service on " + directory.getAbsolutePath());
		
		if (!directory.exists()) {
			Log.e(TAG, "directory, does not exists, making directory");
			directory.mkdirs();
		}
		
		if (!directory.isDirectory()) {
			Log.e(TAG, "path not a directory, or could not make a directory");
			// directory = null;
		} else {
			boolean readable = directory.setReadable(true, true);
			boolean writable = directory.setWritable(true, true);
			
			if (!readable && !writable) {
				Log.e(TAG, "failed to set read/write permissions");
				// Should include some way to handle this situation in the future.
				// Currently it will crash the application with a NullPointerException.
				// Commenting the following line out does not seem to affect anything. 
				// directory = null;
			} else {
				scanSerializedPath();
			}
		}
    }

	public File serialize(ClipData clip) {
		File serialized = null;
		
		if (directory != null && files != null) {
			try {
				final ClipData.Item item = clip.getItemAt(0); // TODO: when do we have multiple clips??
				serialized = new File(directory + "/" + String.format("%x", item.hashCode()) + FILE_SUFFIX);

				final OutputStream out = new BufferedOutputStream(new FileOutputStream(serialized));
				final Parcel parcel = Parcel.obtain();
				
				try {
					parcel.writeValue(clip);
					out.write(parcel.marshall());
				} finally {
					parcel.recycle();
					out.close();
				}
				
				files.add(serialized);
				
			} catch (Exception e) {
				Log.i(TAG, "******************************************************* unable serialize to file!");
			}
		}
		
		return serialized;
	}
	
	public ClipData deserialize(File serialized) {
		ClipData clip = null;
		int bytesRead;
		
		try {
			final InputStream in = new BufferedInputStream(new FileInputStream(serialized));
			final Parcel parcel = Parcel.obtain();
			final byte[] buffer = new byte[READ_BUFFER_SIZE];
			
			try {
				bytesRead = in.read(buffer);
				parcel.unmarshall(buffer, 0, bytesRead);
				parcel.setDataPosition(0);
				
				clip = (ClipData) parcel.readValue(ClipData.class.getClassLoader());
			} finally {
				in.close();
				parcel.recycle();
			}
		} catch (Exception e)	{
			Log.i(TAG, "******************************************************* unable deserialize to file!");	
		}
		
		return clip;
	}
	
	public List<File> scanSerializedPath() {
		files.clear();
		files.addAll(Arrays.asList(directory.listFiles(new FilenameFilter() {
			@Override
			public boolean accept(File dir, String filename) {
				return filename.endsWith(FILE_SUFFIX);
			}
		})));
		return files;
	}
	
	public List<File> getSerializedFiles() {
		return files;
	}
}
