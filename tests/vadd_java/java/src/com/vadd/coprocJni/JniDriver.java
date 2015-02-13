package com.vadd.coprocJni;

public class JniDriver {
	static {
		System.loadLibrary("VaddCoproc");
	}

	public static void main(String[] args) {
		int vecLen = 100;
		if (args.length > 0) {
		    try {
		        vecLen = Integer.parseInt(args[0]);
		    } catch (NumberFormatException e) {
		        System.err.println("Argument" + args[0] + " must be an integer.");
		        System.exit(1);
		    }
		} 
        System.out.println("Running with vecLen = " + vecLen);
        System.out.println("Initializing coproc");
		CoprocJni.initCoproc(vecLen);
		
		System.out.println("Initializing arrays");
        long[] a1 = new long[vecLen];
        long[] a2 = new long[vecLen];
        long[] a3 = new long[vecLen];
        for (int i = 0; i < vecLen; i++) {
        	a1[i] = i;
        	a2[i] = 2 * i;
        	a3[i] = 0;
        }
        
        System.out.println("Calling coproc");
		long actSum = CoprocJni.vadd(vecLen, a1, a2, a3);
        System.out.println("RTN: act_sum = " + actSum);
        // check results
        int errCnt = 0;
        long expSum = 0;
        for (int i = 0; i < vecLen; i++) {
        	if (a3[i] != a1[i] + a2[i]) {
        		System.out.println("a3[" + i + "] is " + a3[i] + " should be " + (a1[i] + a2[i]));
        		errCnt++;
        	}
        	expSum += a1[i] + a2[i];
        }
        if (actSum != expSum) {
        	System.out.println("act_sum " + actSum + " != expSum " + expSum);
        	errCnt++;
        }

        if (errCnt != 0)
        	System.out.println("FAILED: detected " + errCnt + "issues!");
		CoprocJni.exitCoproc();

	}

}
