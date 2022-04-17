import time
import RPi.GPIO as GPIO
import sys
import time
import math


# set up gpio
GPIO.setmode(GPIO.BCM)


sets = [
        [0, 0, 0, 1],
        [0, 0, 1, 1],
        [0, 0, 1, 0],
        [0, 1, 1, 0],
        [0, 1, 0, 0],
        [1, 1, 0, 0],
        [1, 0, 0, 0],
        [1, 0, 0, 1]
    ]

ticksPerRot = 512 * len(sets)


def convertMillisToTicks(millis):
    return millis * ticksPerRot / 16

def convertTicksToMillis(ticks):
    return ticks / ticksPerRot * 16

# does everything in terms of millis externally and ticks internally
class Motor:
    def __init__(self, outs, position):
        self.outs = outs
        self.position = int(convertMillisToTicks(position))
        self.setpoint = int(convertMillisToTicks(position))
        # position % 8 is the current stator state
        # position / 8 / 512 is the current rotation
        for out in self.outs:
            GPIO.setup(out, GPIO.OUT)

    def setSetpoint(self, setpoint):
        self.setpoint = int(convertMillisToTicks(setpoint))
    
    def atSetpoint(self):
        return self.setpoint == self.position

    def stepTowardsSetpoint(self):
        if self.setpoint == self.position:
            return
        
        self.position += 1 if self.setpoint > self.position else -1
        for i in range(4):
            GPIO.output(self.outs[i], GPIO.HIGH if sets[self.position % 8][i] else GPIO.LOW)

    def getSetpoint(self):
        return convertTicksToMillis(self.setpoint)

    def getPosition(self):
        return convertTicksToMillis(self.position)




width  = 245 # measured in millimeters
height = 195 # (0, 0) is top left, (width, -height) is bottom right

x = width / 2 # we're going to start in the middle 
y = -height   # at the bottom

def adjustXYFromUserInput():
    global x
    global y
    print ""
    command = raw_input("=> ")
    if command == "up":
        y += 10
    elif command == "down":
        y -= 10
    elif command == "left":
        x -= 10
    elif command == "right":
        x += 10
    else:
        print "unknown command"


# takes in (x, y) returns [a, b]
def convertXYtoAB(x, y):
    a = math.sqrt(x ** 2 + y ** 2)
    b = math.sqrt((x - width) ** 2 + y ** 2)
    return a, b

initialA, initialB = convertXYtoAB(x, y)



motorA = Motor([12, 16, 20, 21], initialA)
motorB = Motor([25, 8, 7, 1],    initialB)

# do things
try:
    tick = 0
    while True:
        tick += 1
        # then we run 100ms of movement
        adjustXYFromUserInput()
        a, b = convertXYtoAB(x, y)
        print "setpoints are ", a, b
        motorA.setSetpoint(a)
        motorB.setSetpoint(b)
        while (not motorA.atSetpoint()) or (not motorB.atSetpoint()):
            motorA.stepTowardsSetpoint()
            motorB.stepTowardsSetpoint()
            time.sleep(0.001)


except KeyboardInterrupt:
    pass

GPIO.cleanup()








