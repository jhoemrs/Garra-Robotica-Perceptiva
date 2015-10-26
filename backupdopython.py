import time
import serial

ser = serial.Serial('COM4',57600, writeTimeout = 0)

while(1):
    f = open('testeOutput.txt','r')
    line = f.read()
##print(line[8:11])
##4 a 7 RNN
##0 a 3 YNN
##8 a 11 PNN

    if line[0:11]=='YNN:RNN:PNN':
    	print('0')
    	ser.write(b'0')
    elif line[4:7]=='RSL':
    	print('1')
    	ser.write(b'1')
    elif line[4:7]=='RSR':
    	print('2')
    	ser.write(b'2')
    ##elif line[8:11]=='PSU':
    	#print('3')
    ##	ser.write('3')
   ## elif line[8:11]=='PSD':
    	#print('4')
    ##	ser.write('4')

    f.close
    time.sleep(1)

