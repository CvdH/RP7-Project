################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../croutine.c \
../heap_3.c \
../hooks.c \
../i2cMultiMaster.c \
../list.c \
../main.c \
../port.c \
../queue.c \
../serial.c \
../servoPWM.c \
../spi.c \
../tasks.c \
../time.c \
../timers.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
croutine.o \
heap_3.o \
hooks.o \
i2cMultiMaster.o \
list.o \
main.o \
port.o \
queue.o \
serial.o \
servoPWM.o \
spi.o \
tasks.o \
time.o \
timers.o

OBJS_AS_ARGS +=  \
croutine.o \
heap_3.o \
hooks.o \
i2cMultiMaster.o \
list.o \
main.o \
port.o \
queue.o \
serial.o \
servoPWM.o \
spi.o \
tasks.o \
time.o \
timers.o

C_DEPS +=  \
croutine.d \
heap_3.d \
hooks.d \
i2cMultiMaster.d \
list.d \
main.d \
port.d \
queue.d \
serial.d \
servoPWM.d \
spi.d \
tasks.d \
time.d \
timers.d

C_DEPS_AS_ARGS +=  \
croutine.d \
heap_3.d \
hooks.d \
i2cMultiMaster.d \
list.d \
main.d \
port.d \
queue.d \
serial.d \
servoPWM.d \
spi.d \
tasks.d \
time.d \
timers.d

OUTPUT_FILE_PATH +=Temperatuur_luchtvochtigheid.elf

OUTPUT_FILE_PATH_AS_ARGS +=Temperatuur_luchtvochtigheid.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

LINKER_SCRIPT_DEP+= 


# AVR32/GNU C Compiler





























./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR8/GNU C Compiler : 5.4.0
	$(QUOTE)D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe$(QUOTE)  -x c -funsigned-char -funsigned-bitfields -DDEBUG  -I"D:\atmel studio\7.0\Packs\atmel\ATmega_DFP\1.2.150\include"  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=atmega2560 -B "D:\atmel studio\7.0\Packs\atmel\ATmega_DFP\1.2.150\gcc\dev\atmega2560" -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: AVR8/GNU Linker : 5.4.0
	$(QUOTE)D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-g++.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="Temperatuur_luchtvochtigheid.map" -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -mrelax -mmcu=atmega2560 -B "D:\atmel studio\7.0\Packs\atmel\ATmega_DFP\1.2.150\gcc\dev\atmega2560"  
	@echo Finished building target: $@
	"D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "Temperatuur_luchtvochtigheid.elf" "Temperatuur_luchtvochtigheid.hex"
	"D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "Temperatuur_luchtvochtigheid.elf" "Temperatuur_luchtvochtigheid.eep" || exit 0
	"D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objdump.exe" -h -S "Temperatuur_luchtvochtigheid.elf" > "Temperatuur_luchtvochtigheid.lss"
	"D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objcopy.exe" -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "Temperatuur_luchtvochtigheid.elf" "Temperatuur_luchtvochtigheid.srec"
	"D:\atmel studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-size.exe" "Temperatuur_luchtvochtigheid.elf"
	
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "Temperatuur_luchtvochtigheid.elf" "Temperatuur_luchtvochtigheid.a" "Temperatuur_luchtvochtigheid.hex" "Temperatuur_luchtvochtigheid.lss" "Temperatuur_luchtvochtigheid.eep" "Temperatuur_luchtvochtigheid.map" "Temperatuur_luchtvochtigheid.srec" "Temperatuur_luchtvochtigheid.usersignatures"
	