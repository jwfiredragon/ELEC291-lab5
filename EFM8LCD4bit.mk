SHELL=cmd
CC=c51
COMPORT = $(shell type COMPORT.inc)
OBJS=EFM8_ADC.obj lcd.obj

EFM8_ADC.hex: $(OBJS)
	$(CC) $(OBJS)
	@del *.asm *.lst *.lkr 2> nul
	@echo Done!
	
EFM8_ADC.obj: EFM8_ADC.c lcd.h
	$(CC) -c EFM8_ADC.c

lcd.obj: lcd.c lcd.h global.h
	$(CC) -c lcd.c

clean:
	@del $(OBJS) *.asm *.lkr *.lst *.map *.hex *.map 2> nul

LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | sleep 0.5
	EFM8_prog -ft230 -r EFM8_ADC.hex
	cmd /c start putty -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | sleep 0.5
	cmd /c start putty -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

Dummy: EFM8_ADC.hex EFM8_ADC.Map
	@echo Nothing to see here!
	
explorer:
	cmd /c start explorer .
		