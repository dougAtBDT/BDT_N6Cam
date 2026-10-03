################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.c \
../Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.c 

OBJS += \
./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.o \
./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.o 

C_DEPS += \
./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.d \
./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/%.o Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/%.su Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/%.cyclo: ../Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/%.c Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DCMIPP_REGISTER_CALLBACKS=1 -c -I../Core/Inc -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/isp/Inc" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/Components/Common" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/STM32N6570-DK" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/Components/imx335" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/evision/Inc" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/isp/usbx/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STM32_ISP_Library-2f-isp-2f-usbx-2f-Src

clean-Middlewares-2f-ST-2f-STM32_ISP_Library-2f-isp-2f-usbx-2f-Src:
	-$(RM) ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.d ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.o ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.su ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.d ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.o ./Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.su

.PHONY: clean-Middlewares-2f-ST-2f-STM32_ISP_Library-2f-isp-2f-usbx-2f-Src

