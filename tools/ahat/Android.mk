#
# Copyright (C) 2015 The Android Open Source Project
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
#

LOCAL_PATH := $(call my-dir)

include art/build/Android.common_path.mk

# --- ahat.jar ----------------
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAR_MANIFEST := src/manifest.txt
LOCAL_JAVA_RESOURCE_FILES := $(LOCAL_PATH)/src/style.css
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := ahat

# Let users with Java 7 run ahat (b/28303627)
LOCAL_JAVA_LANGUAGE_VERSION := 1.7

include $(BUILD_HOST_JAVA_LIBRARY)

# --- ahat script ----------------
include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE := ahat
LOCAL_SRC_FILES := ahat
include $(BUILD_PREBUILT)

# The ahat tests rely on running ART to generate a heap dump for test, but ART
# doesn't run on darwin. Only build and run the tests for linux.
# There are also issues with running under instrumentation.
ifeq ($(HOST_OS),linux)
ifneq ($(EMMA_INSTRUMENT),true)
# --- ahat-test-dump.jar --------------
include $(CLEAR_VARS)
LOCAL_MODULE := ahat-test-dump
LOCAL_MODULE_TAGS := tests
LOCAL_SRC_FILES := $(call all-java-files-under, test-dump)
LOCAL_PROGUARD_ENABLED := obfuscation
LOCAL_PROGUARD_FLAG_FILES := test-dump/config.pro
include $(BUILD_JAVA_LIBRARY)

# Determine the location of the test-dump.jar, test-dump.hprof, and proguard
# map files. These use variables set implicitly by the include of
# BUILD_JAVA_LIBRARY above.
AHAT_TEST_DUMP_JAR := $(LOCAL_BUILT_MODULE)
AHAT_TEST_DUMP_HPROF := $(intermediates.COMMON)/test-dump.hprof
AHAT_TEST_DUMP_BASE_HPROF := $(intermediates.COMMON)/test-dump-base.hprof
AHAT_TEST_DUMP_PROGUARD_MAP := $(intermediates.COMMON)/test-dump.map

# Generate the proguard map in the desired location by copying it from
# wherever the build system generates it by default.
$(AHAT_TEST_DUMP_PROGUARD_MAP): PRIVATE_AHAT_SOURCE_PROGUARD_MAP := $(proguard_dictionary)
$(AHAT_TEST_DUMP_PROGUARD_MAP): $(proguard_dictionary)
	cp $(PRIVATE_AHAT_SOURCE_PROGUARD_MAP) $@

# Run ahat-test-dump.jar to generate test-dump.hprof and test-dump-base.hprof
AHAT_TEST_DUMP_DEPENDENCIES := \
  $(ART_HOST_EXECUTABLES) \
  $(ART_HOST_SHARED_LIBRARY_DEPENDENCIES) \
  $(HOST_OUT_EXECUTABLES)/art \
  $(HOST_CORE_IMG_OUT_BASE)$(CORE_IMG_SUFFIX)

$(AHAT_TEST_DUMP_HPROF): PRIVATE_AHAT_TEST_ART := $(HOST_OUT_EXECUTABLES)/art
$(AHAT_TEST_DUMP_HPROF): PRIVATE_AHAT_TEST_DUMP_JAR := $(AHAT_TEST_DUMP_JAR)
$(AHAT_TEST_DUMP_HPROF): $(AHAT_TEST_DUMP_JAR) $(AHAT_TEST_DUMP_DEPENDENCIES)
	$(PRIVATE_AHAT_TEST_ART) -cp $(PRIVATE_AHAT_TEST_DUMP_JAR) Main $@

$(AHAT_TEST_DUMP_BASE_HPROF): PRIVATE_AHAT_TEST_ART := $(HOST_OUT_EXECUTABLES)/art
$(AHAT_TEST_DUMP_BASE_HPROF): PRIVATE_AHAT_TEST_DUMP_JAR := $(AHAT_TEST_DUMP_JAR)
$(AHAT_TEST_DUMP_BASE_HPROF): $(AHAT_TEST_DUMP_JAR) $(AHAT_TEST_DUMP_DEPENDENCIES)
	$(PRIVATE_AHAT_TEST_ART) -cp $(PRIVATE_AHAT_TEST_DUMP_JAR) Main $@ --base

# --- ahat-tests.jar --------------
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, test)
LOCAL_JAR_MANIFEST := test/manifest.txt
LOCAL_JAVA_RESOURCE_FILES := \
  $(AHAT_TEST_DUMP_HPROF) \
  $(AHAT_TEST_DUMP_BASE_HPROF) \
  $(AHAT_TEST_DUMP_PROGUARD_MAP) \
  $(LOCAL_PATH)/test-dump/L.hprof \
  $(LOCAL_PATH)/test-dump/O.hprof \
  $(LOCAL_PATH)/test-dump/RI.hprof
LOCAL_STATIC_JAVA_LIBRARIES := ahat junit-host
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE := ahat-tests
include $(BUILD_HOST_JAVA_LIBRARY)
AHAT_TEST_JAR := $(LOCAL_BUILT_MODULE)

.PHONY: ahat-test
ahat-test: PRIVATE_AHAT_TEST_JAR := $(AHAT_TEST_JAR)
ahat-test: $(AHAT_TEST_JAR)
	java -enableassertions -jar $(PRIVATE_AHAT_TEST_JAR)
endif # EMMA_INSTRUMENT
endif # linux

# Clean up local variables.
AHAT_TEST_JAR :=
AHAT_TEST_DUMP_JAR :=
AHAT_TEST_DUMP_HPROF :=
AHAT_TEST_DUMP_BASE_HPROF :=
AHAT_TEST_DUMP_PROGUARD_MAP :=
AHAT_TEST_DUMP_DEPENDENCIES :=

