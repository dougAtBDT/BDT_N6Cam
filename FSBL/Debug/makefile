################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/ST/STM32_ISP_Library/isp/Src/subdir.mk
-include Drivers/STM32N6xx_HAL_Driver/Src/subdir.mk
-include Drivers/STM32N6xx_HAL_Driver/subdir.mk
-include Drivers/BSP/STM32N6570-DK/subdir.mk
-include Drivers/BSP/Components/mx66uw1g45g/subdir.mk
-include Drivers/BSP/Components/imx335/subdir.mk
-include Drivers/BSP/Components/aps256xx/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := BDT_N6Cam_03_FSBL
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
BDT_N6Cam_03_FSBL.elf \

MAP_FILES += \
BDT_N6Cam_03_FSBL.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
BDT_N6Cam_03_FSBL.list \

OBJCOPY_BIN += \
BDT_N6Cam_03_FSBL.bin \


# All Target
all: main-build

# Main-build Target
main-build: BDT_N6Cam_03_FSBL.elf secondary-outputs

# Tool invocations
BDT_N6Cam_03_FSBL.elf BDT_N6Cam_03_FSBL.map: $(OBJS) $(USER_OBJS) C:\Users\doug\STM32CubeIDE\workspace_1.16.1\BDT_N6Cam_03\FSBL\STM32N657X0HXQ_AXISRAM2_fsbl.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "BDT_N6Cam_03_FSBL.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m55 -T"C:\Users\doug\STM32CubeIDE\workspace_1.16.1\BDT_N6Cam_03\FSBL\STM32N657X0HXQ_AXISRAM2_fsbl.ld" --specs=nosys.specs -Wl,-Map="BDT_N6Cam_03_FSBL.map" -Wl,--gc-sections -static -Wl,--cmse-implib -Wl,--out-implib=./secure_nsclib.o -L"C:\Users\doug\STM32CubeIDE\workspace_1.16.1\BDT_N6Cam_03\FSBL\Middlewares\ST\STM32_ISP_Library\evision\Lib" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

BDT_N6Cam_03_FSBL.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "BDT_N6Cam_03_FSBL.list"
	@echo 'Finished building: $@'
	@echo ' '

BDT_N6Cam_03_FSBL.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "BDT_N6Cam_03_FSBL.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) BDT_N6Cam_03_FSBL.bin BDT_N6Cam_03_FSBL.elf BDT_N6Cam_03_FSBL.list BDT_N6Cam_03_FSBL.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
