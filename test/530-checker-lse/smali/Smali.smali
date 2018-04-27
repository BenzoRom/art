# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
.class public LSmali;
.super Ljava/lang/Object;


##  CHECK-START: boolean Smali.booleanFillArrayNegLSE(boolean[]) builder (after)
##  CHECK-DAG: <<ConstPos:i\d+>> IntConstant 255
##  CHECK-DAG: <<Const1:i\d+>>   IntConstant 1
##  CHECK-DAG:                   ArraySet [{{l\d+}},{{i\d+}},<<ConstPos>>]
##  CHECK-DAG: <<AGet:z\d+>>     ArrayGet
##  CHECK-DAG:                   Return [<<AGet>>]

##  CHECK-START: boolean Smali.booleanFillArrayNegLSE(boolean[]) load_store_elimination (after)
##  CHECK-DAG: <<ConstPos:i\d+>> IntConstant 255
##  CHECK-DAG: <<Const1:i\d+>>   IntConstant 1
##  CHECK-DAG:                   ArraySet [{{l\d+}},{{i\d+}},<<ConstPos>>]
##  CHECK-DAG:                   Return [<<ConstPos>>]
#
##  CHECK-NOT:                   ArrayGet
.method public static booleanFillArrayNegLSE([Z)Z
   .registers 2
   fill-array-data v1, :ArrayData
   const/4 v0, 0x1
   aget-boolean v0, v1, v0
   return v0
:ArrayData
    .array-data 1
        0 -1 24
    .end array-data
.end method


##  CHECK-START: char Smali.charFillArrayNegLSE(char[]) builder (after)
##  CHECK-DAG: <<ConstPos:i\d+>> IntConstant 65535
##  CHECK-DAG: <<Const1:i\d+>>   IntConstant 1
##  CHECK-DAG:                   ArraySet [{{l\d+}},{{i\d+}},<<ConstPos>>]
##  CHECK-DAG: <<AGet:c\d+>>     ArrayGet
##  CHECK-DAG:                   Return [<<AGet>>]

##  CHECK-START: char Smali.charFillArrayNegLSE(char[]) load_store_elimination (after)
##  CHECK-DAG: <<ConstPos:i\d+>> IntConstant 65535
##  CHECK-DAG: <<Const1:i\d+>>   IntConstant 1
##  CHECK-DAG:                   ArraySet [{{l\d+}},{{i\d+}},<<ConstPos>>]
##  CHECK-DAG: <<Conv:c\d+>>     TypeConversion [<<ConstPos>>]
##  CHECK-DAG:                   Return [<<Conv>>]
##  CHECK-NOT:                   ArrayGet
.method public static charFillArrayNegLSE([C)C
   .registers 2
   fill-array-data v1, :ArrayData
   const/4 v0, 0x1
   aget-char v0, v1, v0
   return v0
:ArrayData
    .array-data 2
        0 -1 24
    .end array-data
.end method
