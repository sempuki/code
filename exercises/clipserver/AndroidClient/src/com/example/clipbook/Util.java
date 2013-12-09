package com.example.clipbook;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Random;

public class Util {
	public static Long generateRand64() {
		return new Random().nextLong();
	}

	public static Long generateRand63() {
		return generateRand64() & 0x7FFFFFFFFFFFFFFFL;
	}
	
	public static Long generateHash64(String phrase) {
		long result = 0;
		byte[] output, input;
		try {
			input = phrase.getBytes("UTF-8");
			output = MessageDigest.getInstance("SHA-256").digest(input);
			ByteBuffer buffer = ByteBuffer.wrap(output);
			buffer.order(ByteOrder.BIG_ENDIAN);
			result = buffer.getLong() ^ 
					buffer.getLong() ^ 
					buffer.getLong() ^
					buffer.getLong(); // 256 bits XORed together
		}
		catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
		}
		return result;
	}
	
	public static Long generateHash63(String phrase) {
		return generateHash64(phrase) & 0x7FFFFFFFFFFFFFFFL;
	}
}
