/*******************************************************************************
 * Copyright (c) 2001, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/
package com.ibm.j9.jsr292.indyn;

public class Helper {
	
	public int publicInt = 1; 
	
	public static int publicStaticInt = 100;

	public static void voidMethod() { }
	
	public static void addStaticPublic(int a, String b){ }
	
	public static int negativeInt(int i) { return -i; }
	
	public static boolean isDivisibleByThree ( int n ) {
		return (n % 3 == 0);
	}
	
	public static boolean isDivisibleByFive ( int n ) {
		return (n % 5 == 0);
	}
	
	public static String returnFizz (int i) {
		return "fizz";
	}
	
	public static String returnBuzz (int i) {
		return "buzz";
	}
	
	public static String returnNothing (int i) {
		return "nothing";
	}
	
	private static String staticPrivateMethod(){
		return "I am a private static method";
	}
}
