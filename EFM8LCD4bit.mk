SHELL=cmd
CC=c51
COMPORT = $(shell type COMPORT.inc)
OBJS=PeriodEFM8.obj lcd.obj

PeriodEFM8.hex: $(OBJS)
	$(CC) $(OBJS)
	@del *.asm *.lst *.lkr 2> nul
	@echo Done!
	
PeriodEFM8.obj: PeriodEFM8.c lcd.h
	$(CC) -c PeriodEFM8.c

lcd.obj: lcd.c lcd.h global.h
	$(CC) -c lcd.c

clean:
	@del $(OBJS) *.asm *.lkr *.lst *.map *.hex *.map 2> nul

LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | sleep 0.5
	EFM8_prog -ft230 -r PeriodEFM8.hex
	cmd /c start putty -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | sleep 0.5
	cmd /c start putty -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

Dummy: PeriodEFM8.hex PeriodEFM8.Map
	@echo Nothing to see here!
	
explorer:
	cmd /c start explorer .
		