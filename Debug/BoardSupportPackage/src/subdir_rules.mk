################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
BoardSupportPackage/src/%.obj: ../BoardSupportPackage/src/%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/Users/Ryan/workspace_v8/lab6/G8RTOS" --include_path="C:/ti/ccsv8/ccs_base/arm/include" --include_path="C:/Users/Ryan/workspace_v8/lab6/CC3100SupportPackage/SL_Common" --include_path="C:/Users/Ryan/workspace_v8/lab6/CC3100SupportPackage/spi_cc3100" --include_path="C:/Users/Ryan/workspace_v8/lab6/CC3100SupportPackage/cc3100_usage" --include_path="C:/Users/Ryan/workspace_v8/lab6/CC3100SupportPackage/board" --include_path="C:/Users/Ryan/workspace_v8/lab6/CC3100SupportPackage/simplelink/include" --include_path="C:/Users/Ryan/workspace_v8/lab6/BoardSupportPackage/DriverLib" --include_path="C:/Users/Ryan/workspace_v8/lab6/BoardSupportPackage/inc" --include_path="C:/ti/ccsv8/ccs_base/arm/include/CMSIS" --include_path="C:/Users/Ryan/workspace_v8/lab6" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --advice:power=all --define=__MSP432P401R__ --define=ccs -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="BoardSupportPackage/src/$(basename $(<F)).d_raw" --obj_directory="BoardSupportPackage/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


