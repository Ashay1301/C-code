import RPi.GPIO as GPIO
import time
import pandas as pd
#import pygame

#pygame.mixer.init()
#pygame.mixer.music.load("../sounds/ding.mp3")

# change these as desired - they're the pins connected from the
# SPI port on the ADC to the Cobbler
SPICLK = 11
SPIMISO = 9
SPIMOSI = 10
SPICS = 8
mq2_dpin = 26
mq2_apin = 0
buz_pin = 20
df = pd.read_csv("data.csv")

#port init
def init():
         GPIO.setwarnings(False)
         GPIO.cleanup()			#clean up at the end of your script
         GPIO.setmode(GPIO.BCM)		#to specify whilch pin numbering system
         # set up the SPI interface pins
         GPIO.setup(SPIMOSI, GPIO.OUT)
         GPIO.setup(SPIMISO, GPIO.IN)
         GPIO.setup(SPICLK, GPIO.OUT)
         GPIO.setup(SPICS, GPIO.OUT)
         GPIO.setup(mq2_dpin,GPIO.IN,pull_up_down=GPIO.PUD_DOWN)
         GPIO.setup(buz_pin, GPIO.OUT)

#read SPI data from MCP3008(or MCP3204) chip,8 possible adc's (0 thru 7)
def readadc(adcnum, clockpin, mosipin, misopin, cspin):
        if ((adcnum > 7) or (adcnum < 0)):
                return -1
        GPIO.output(cspin, True)	

        GPIO.output(clockpin, False)  # start clock low
        GPIO.output(cspin, False)     # bring CS low

        commandout = adcnum
        commandout |= 0x18  # start bit + single-ended bit
        commandout <<= 3    # we only need to send 5 bits here
        for i in range(5):
                if (commandout & 0x80):
                        GPIO.output(mosipin, True)
                else:
                        GPIO.output(mosipin, False)
                commandout <<= 1
                GPIO.output(clockpin, True)
                GPIO.output(clockpin, False)

        adcout = 0
        # read in one empty bit, one null bit and 10 ADC bits
        for i in range(12):
                GPIO.output(clockpin, True)
                GPIO.output(clockpin, False)
                adcout <<= 1
                if (GPIO.input(misopin)):
                        adcout |= 0x1

        GPIO.output(cspin, True)
        
        adcout >>= 1       # first bit is 'null' so drop it
        return adcout
#main ioop
def main():
         init()
         print("please wait...")
         time.sleep(20)
         for i in range(10):
                  COlevel=readadc(mq2_apin, SPICLK, SPIMOSI, SPIMISO, SPICS)
                  val = ((COlevel/1024.)*3.3)
                  print("Current Gas AD value = " +str("%.2f"% val)+" V")
                  if GPIO.input(mq2_dpin):
                           print("No gas")
                           df.loc[3,"Gas"] = 'No Gas'
                           time.sleep(1)
                  else:
                           val = ((COlevel/1024.)*3.3)
                           if val > 2 and val < 3.7:
                               pygame.mixer.music.play()
                               GPIO.output(buz_pin,GPIO.LOW)
                               time.sleep(0.1)
                               GPIO.output(buz_pin,GPIO.HIGH)
                           print("Current Gas AD value = " +str("%.2f"% val)+" V")
                           df.loc[3,"Gas"] = 'Gas Detected'
                           time.sleep(1)

if __name__ =='__main__':
         try:
                  main()
                  pass
         except KeyboardInterrupt:
                  pass

GPIO.cleanup()