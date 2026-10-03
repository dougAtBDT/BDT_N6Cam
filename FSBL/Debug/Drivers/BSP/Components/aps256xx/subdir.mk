################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/Components/aps256xx/aps256xx.c 

OBJS += \
./Drivers/BSP/Components/aps256xx/aps256xx.o 

C_DEPS += \
./Drivers/BSP/Components/aps256xx/aps256xx.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/aps256xx/%.o Drivers/BSP/Components/aps256xx/%.su Drivers/BSP/Components/aps256xx/%.cyclo: ../Drivers/BSP/Components/aps256xx/%.c Drivers/BSP/Components/aps256xx/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DCMIPP_REGISTER_CALLBACKS=1 -c -I../Core/Inc -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/isp/Inc" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/Components/Common" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/STM32N6570-DK" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/Components/imx335" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/evision/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Components-2f-aps256xx

clean-Drivers-2f-BSP-2f-Components-2f-aps256xx:
	-$(RM) ./Drivers/BSP/Components/aps256xx/aps256xx.cyclo ./Drivers/BSP/Components/aps256xx/aps256xx.d ./Drivers/BSP/Components/aps256xx/aps256xx.o ./Drivers/BSP/Components/aps256xx/aps256xx.su

.PHONY: clean-Drivers-2f-BSP-2f-Components-2f-aps256xx

