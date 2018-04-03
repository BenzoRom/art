/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Functional tests for detecting min/max.
 */
public class Main {

  //
  // Direct intrinsics.
  //

  /// CHECK-START: int Main.minI(int) instruction_simplifier (before)
  /// CHECK-DAG: <<Par:i\d+>> ParameterValue
  /// CHECK-DAG: <<Con:i\d+>> IntConstant 20
  /// CHECK-DAG: <<Min:i\d+>> InvokeStaticOrDirect [<<Par>>,<<Con>>] intrinsic:MathMinIntInt
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.minI(int) instruction_simplifier (after)
  /// CHECK-DAG: <<Par:i\d+>> ParameterValue
  /// CHECK-DAG: <<Con:i\d+>> IntConstant 20
  /// CHECK-DAG: <<Min:i\d+>> Min [<<Par>>,<<Con>>]
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.minI(int) instruction_simplifier (after)
  /// CHECK-NOT:              InvokeStaticOrDirect
  public static int minI(int a) {
    return Math.min(a, 20);
  }

  /// CHECK-START: long Main.minL(long) instruction_simplifier (before)
  /// CHECK-DAG: <<Par:j\d+>> ParameterValue
  /// CHECK-DAG: <<Con:j\d+>> LongConstant 20
  /// CHECK-DAG: <<Min:j\d+>> InvokeStaticOrDirect [<<Par>>,<<Con>>] intrinsic:MathMinLongLong
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: long Main.minL(long) instruction_simplifier (after)
  /// CHECK-DAG: <<Par:j\d+>> ParameterValue
  /// CHECK-DAG: <<Con:j\d+>> LongConstant 20
  /// CHECK-DAG: <<Min:j\d+>> Min [<<Par>>,<<Con>>]
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: long Main.minL(long) instruction_simplifier (after)
  /// CHECK-NOT:              InvokeStaticOrDirect
  public static long minL(long a) {
    return Math.min(a, 20L);
  }

  /// CHECK-START: int Main.maxI(int) instruction_simplifier (before)
  /// CHECK-DAG: <<Par:i\d+>> ParameterValue
  /// CHECK-DAG: <<Con:i\d+>> IntConstant 20
  /// CHECK-DAG: <<Max:i\d+>> InvokeStaticOrDirect [<<Par>>,<<Con>>] intrinsic:MathMaxIntInt
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.maxI(int) instruction_simplifier (after)
  /// CHECK-DAG: <<Par:i\d+>> ParameterValue
  /// CHECK-DAG: <<Con:i\d+>> IntConstant 20
  /// CHECK-DAG: <<Max:i\d+>> Max [<<Par>>,<<Con>>]
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.maxI(int) instruction_simplifier (after)
  /// CHECK-NOT:              InvokeStaticOrDirect
  public static int maxI(int a) {
    return Math.max(a, 20);
  }

  /// CHECK-START: long Main.maxL(long) instruction_simplifier (before)
  /// CHECK-DAG: <<Par:j\d+>> ParameterValue
  /// CHECK-DAG: <<Con:j\d+>> LongConstant 20
  /// CHECK-DAG: <<Max:j\d+>> InvokeStaticOrDirect [<<Par>>,<<Con>>] intrinsic:MathMaxLongLong
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: long Main.maxL(long) instruction_simplifier (after)
  /// CHECK-DAG: <<Par:j\d+>> ParameterValue
  /// CHECK-DAG: <<Con:j\d+>> LongConstant 20
  /// CHECK-DAG: <<Max:j\d+>> Max [<<Par>>,<<Con>>]
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: long Main.maxL(long) instruction_simplifier (after)
  /// CHECK-NOT:              InvokeStaticOrDirect
  public static long maxL(long a) {
    return Math.max(a, 20L);
  }

  //
  // Different types.
  //

  /// CHECK-START: int Main.min1(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> GreaterThanOrEqual [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.min1(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:i\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.min1(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int min1(int a, int b) {
    return a < b ? a : b;
  }

  /// CHECK-START: int Main.min2(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> GreaterThan [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.min2(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:i\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.min2(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int min2(int a, int b) {
    return a <= b ? a : b;
  }

  /// CHECK-START: int Main.min3(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThanOrEqual [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.min3(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:i\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.min3(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int min3(int a, int b) {
    return a > b ? b : a;
  }

  /// CHECK-START: int Main.min4(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.min4(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:i\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.min4(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int min4(int a, int b) {
    return a >= b ? b : a;
  }

  /// CHECK-START: int Main.min5(short, short) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:s\d+>>,<<Op2:s\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.min5(short, short) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:i\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.min5(short, short) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int min5(short a, short b) {
    return a >= b ? b : a;
  }

  /// CHECK-START: int Main.min6(byte, byte) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:b\d+>>,<<Op2:b\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.min6(byte, byte) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:i\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: int Main.min6(byte, byte) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int min6(byte a, byte b) {
    return a >= b ? b : a;
  }

  /// CHECK-START: long Main.min7(long, long) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:j\d+>>,<<Op2:j\d+>>]
  /// CHECK-DAG: <<Sel:j\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: long Main.min7(long, long) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Min:j\d+>> Min
  /// CHECK-DAG:              Return [<<Min>>]
  //
  /// CHECK-START: long Main.min7(long, long) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static long min7(long a, long b) {
    return a >= b ? b : a;
  }

  /// CHECK-START: int Main.max1(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> GreaterThanOrEqual [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.max1(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:i\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.max1(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int max1(int a, int b) {
    return a < b ? b : a;
  }

  /// CHECK-START: int Main.max2(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> GreaterThan [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op2>>,<<Op1>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.max2(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:i\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.max2(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int max2(int a, int b) {
    return a <= b ? b : a;
  }

  /// CHECK-START: int Main.max3(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThanOrEqual [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.max3(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:i\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.max3(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int max3(int a, int b) {
    return a > b ? a : b;
  }

  /// CHECK-START: int Main.max4(int, int) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:i\d+>>,<<Op2:i\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.max4(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:i\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.max4(int, int) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int max4(int a, int b) {
    return a >= b ? a : b;
  }

  /// CHECK-START: int Main.max5(short, short) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:s\d+>>,<<Op2:s\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.max5(short, short) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:i\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.max5(short, short) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int max5(short a, short b) {
    return a >= b ? a : b;
  }

  /// CHECK-START: int Main.max6(byte, byte) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:b\d+>>,<<Op2:b\d+>>]
  /// CHECK-DAG: <<Sel:i\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: int Main.max6(byte, byte) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:i\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: int Main.max6(byte, byte) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static int max6(byte a, byte b) {
    return a >= b ? a : b;
  }

  /// CHECK-START: long Main.max7(long, long) instruction_simplifier$after_inlining (before)
  /// CHECK-DAG: <<Cnd:z\d+>> LessThan [<<Op1:j\d+>>,<<Op2:j\d+>>]
  /// CHECK-DAG: <<Sel:j\d+>> Select [<<Op1>>,<<Op2>>,<<Cnd>>]
  /// CHECK-DAG:              Return [<<Sel>>]
  //
  /// CHECK-START: long Main.max7(long, long) instruction_simplifier$after_inlining (after)
  /// CHECK-DAG: <<Max:j\d+>> Max
  /// CHECK-DAG:              Return [<<Max>>]
  //
  /// CHECK-START: long Main.max7(long, long) instruction_simplifier$after_inlining (after)
  /// CHECK-NOT:              Select
  public static long max7(long a, long b) {
    return a >= b ? a : b;
  }

  public static void main(String[] args) {
    // Types.
    expectEquals(10, minI(10));
    expectEquals(20, minI(25));
    expectEquals(10L, minL(10L));
    expectEquals(20L, minL(25L));
    expectEquals(20, maxI(10));
    expectEquals(25, maxI(25));
    expectEquals(20L, maxL(10L));
    expectEquals(25L, maxL(25L));
    expectEquals(10, min1(10, 20));
    expectEquals(10, min2(10, 20));
    expectEquals(10, min3(10, 20));
    expectEquals(10, min4(10, 20));
    expectEquals(10, min5((short) 10, (short) 20));
    expectEquals(10, min6((byte) 10, (byte) 20));
    expectEquals(10L, min7(10L, 20L));
    expectEquals(20, max1(10, 20));
    expectEquals(20, max2(10, 20));
    expectEquals(20, max3(10, 20));
    expectEquals(20, max4(10, 20));
    expectEquals(20, max5((short) 10, (short) 20));
    expectEquals(20, max6((byte) 10, (byte) 20));
    expectEquals(20L, max7(10L, 20L));
    System.out.println("passed");
  }

  private static void expectEquals(int expected, int result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }

  private static void expectEquals(long expected, long result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }
}
