################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/unc/miniz.c 

OBJS += \
./src/unc/miniz.o 

C_DEPS += \
./src/unc/miniz.d 


# Each subdirectory must supply rules for building sources it contributes
src/unc/%.o: ../src/unc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-mingw32-g++ -DMINGW32 -DSDL -DGLEW_STATIC -I/home/fhorse/sviluppo/personale/punes/src -I/home/fhorse/sviluppo/personale/punes/src/sdl -I/usr/i686-mingw32/usr/include/SDL -I/home/fhorse/sviluppo/personale/punes/misc/DXSDK/Include -I/home/fhorse/sviluppo/personale/punes/misc/DXSDK/vc10/include -O3 -Wall -Wconversion -ffast-math -c -fmessage-length=0 -finline-functions -Winline -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

