################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include tools/subdir.mk
-include obj/sign/tools/subdir.mk
-include obj/libs/subdir.mk
-include obj/kern/trap/subdir.mk
-include obj/kern/mm/subdir.mk
-include obj/kern/libs/subdir.mk
-include obj/kern/init/subdir.mk
-include obj/kern/driver/subdir.mk
-include obj/kern/debug/subdir.mk
-include obj/boot/subdir.mk
-include obj/subdir.mk
-include libs/subdir.mk
-include kern/trap/subdir.mk
-include kern/mm/subdir.mk
-include kern/libs/subdir.mk
-include kern/init/subdir.mk
-include kern/driver/subdir.mk
-include kern/debug/subdir.mk
-include boot/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lab1_result

# Tool invocations
lab1_result: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "lab1_result" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) lab1_result
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
