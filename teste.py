import time
import serial

ser = serial.Serial('COM3',9600, writeTimeout = 0)
##Valor que nao iremos chegar 8
x=b'8'
y=b'8'
while(1):
    f = open('testeOutput.txt','r')
    line = f.read()
    ##4 a 7 RNN
    ##0 a 3 YNN
    ##8 a 11 PNN
    ser.flush()
    ser.flushInput()
    ser.flushOutput()

    if x == y:
        print('Em Espera: ')
    if line[0:11]=='YNN:RNN:PNN':
        z = b'0'
        if x != z:
            x = b'0'
            print('0')
            ser.write(x)
    elif line[4:7]=='RSL':
        z = b'1'
        if x != z:
            x = b'1'
            print('1')
            ser.write(x)
    elif line[4:7]=='RSR':
        z = b'2'
        if x != z:
            x = b'2'
            print('2')
            ser.write(x)
    elif line[8:11]=='PSD':
        z = b'3'
        if x !=z :
            x = b'3'
            print('3')
            ser.write('3')
    elif line[8:11]=='PSU':
        z = b'4'
        if x !=z :
            x = b'4'
            print('4')
            ser.write('4')

    f.close
    time.sleep(1)

