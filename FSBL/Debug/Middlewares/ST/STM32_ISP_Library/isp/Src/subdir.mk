################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.c \
../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.c 

OBJS += \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.o \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.o 

C_DEPS += \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.d \
./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STM32_ISP_Library/isp/Src/%.o Middlewares/ST/STM32_ISP_Library/isp/Src/%.su Middlewares/ST/STM32_ISP_Library/isp/Src/%.cyclo: ../Middlewares/ST/STM32_ISP_Library/isp/Src/%.c Middlewares/ST/STM32_ISP_Library/isp/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DCMIPP_REGISTER_CALLBACKS=1 -c -I../Core/Inc -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/isp/Inc" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/Components/Common" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/STM32N6570-DK" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Drivers/BSP/Components/imx335" -I"C:/Users/doug/STM32CubeIDE/workspace_1.16.1/BDT_N6Cam_03/FSBL/Middlewares/ST/STM32_ISP_Library/evision/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STM32_ISP_Library-2f-isp-2f-Src

clean-Middlewares-2f-ST-2f-STM32_ISP_Library-2f-isp-2f-Src:
	-$(RM) ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_conf_template.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.su ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.cyclo ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.d ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.o ./Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.su

.PHONY: clean-Middlewares-2f-ST-2f-STM32_ISP_Library-2f-isp-2f-Src

