package com.vadd.coprocJni;
// execute following from java/src directory
// javah -d ../../src com.vadd.coprocJni.CoprocJni

public class CoprocJni {
	public static native int initCoproc(long vecLen);
	public static native void exitCoproc();
	public static native long vadd(long vecLen,
			long[] a1,
			long[] a2,
			long[] a3);

}
