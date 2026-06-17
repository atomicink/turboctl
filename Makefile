PROJECT := turboctl
VERSION := 0.1.0
BUNDLE_ID := dev.yu.turboctl
KEXT_NAME := TurboCtl

BUILD_DIR := build
SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null)
KERNEL_SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-platform-path 2>/dev/null)/Developer/SDKs/MacOSX.sdk
KERNEL_HEADERS := $(KERNEL_SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers
USER_ARCHS ?= x86_64 arm64
USER_ARCH_FLAGS := $(foreach arch,$(USER_ARCHS),-arch $(arch))

CC := xcrun clang

KEXT_CFLAGS := \
	-arch x86_64 \
	-isysroot $(KERNEL_SDKROOT) \
	-I$(KERNEL_HEADERS) \
	-mmacosx-version-min=10.13 \
	-mkernel \
	-DKERNEL \
	-DKERNEL_PRIVATE \
	-fno-builtin \
	-fno-common \
	-fno-stack-protector \
	-fno-strict-aliasing \
	-Wall \
	-Wextra \
	-Werror

KEXT_LDFLAGS := \
	-arch x86_64 \
	-isysroot $(KERNEL_SDKROOT) \
	-mmacosx-version-min=10.13 \
	-nostdlib \
	-r \
	-Wl,-kext

USER_CFLAGS := \
	$(USER_ARCH_FLAGS) \
	-isysroot $(SDKROOT) \
	-mmacosx-version-min=10.13 \
	-O2 \
	-Wall \
	-Wextra \
	-Werror

KEXT_BUNDLE := $(BUILD_DIR)/$(KEXT_NAME).kext
KEXT_EXEC := $(KEXT_BUNDLE)/Contents/MacOS/$(KEXT_NAME)

.PHONY: all kext tools clean print-sdk

all: kext tools

print-sdk:
	@echo "SDKROOT=$(SDKROOT)"
	@echo "KERNEL_SDKROOT=$(KERNEL_SDKROOT)"
	@echo "KERNEL_HEADERS=$(KERNEL_HEADERS)"

kext: $(KEXT_EXEC)

tools: $(BUILD_DIR)/turboctl $(BUILD_DIR)/turboctld

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/obj:
	mkdir -p $@

$(KEXT_BUNDLE)/Contents/MacOS:
	mkdir -p $@

$(KEXT_BUNDLE)/Contents/Info.plist: kext/Info.plist | $(KEXT_BUNDLE)/Contents/MacOS
	cp $< $@

$(BUILD_DIR)/obj/TurboCtl.o: kext/src/TurboCtl.c | $(BUILD_DIR)/obj
	$(CC) $(KEXT_CFLAGS) -c $< -o $@

$(KEXT_EXEC): $(BUILD_DIR)/obj/TurboCtl.o $(KEXT_BUNDLE)/Contents/Info.plist
	$(CC) $(KEXT_LDFLAGS) $< -lkmod -lcc_kext -o $@

$(BUILD_DIR)/turboctl: tools/turboctl/turboctl.c | $(BUILD_DIR)
	$(CC) $(USER_CFLAGS) $< -o $@

$(BUILD_DIR)/turboctld: daemon/turboctld/turboctld.c | $(BUILD_DIR)
	$(CC) $(USER_CFLAGS) $< -framework IOKit -framework CoreFoundation -o $@

clean:
	rm -rf $(BUILD_DIR)
