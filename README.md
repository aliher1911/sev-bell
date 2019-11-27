# Sev Bell
Sketch for servo control

## Functionality
Board could play a sequence of servo positions with a varying delay between them.  
Each (position, delay) tuple is called action in this doc. Position could be a number in the range of 0, 180 degrees inclusive. Delay could be a number in range of 0, 60000 milliseconds inclusive.  
Board holds a single sequence of actions to play.  
Maximum number of actions in sequence is 16.  
Tuple with (-1,0) will end sequence early if not all slots are used.  
Board starts with initial programmed sequence which can be reprogrammed, but the change is not persistent and have to be reapplied after every restart.

## Initial state
Initial position of servo is 90 degrees (middle of range).  
Initial sequence is:  
(30, 150)  
(135, 300)  
(90, 150)  

## Communication
Board listens for commands on usb serial port with baud rate 9600.
Each command is a string terminating with \n or \r not containing 0's
If byte 0 is received, then current line is dropped and communication is restarted

## Commands 
Format of the command string:  
&lt;code&gt;[:&lt;val&gt;[,&lt;val&gt;]]  
code - command code  
val - list of optional values that command support

### Start servo sequence
Code = '1'  
Start playback sequence. If previous sequence is still running next run is eqnueued to start as soon as previous completes.

### Stop running sequence
Code = '0'  
Stop currently running sequence if there's one.

### Change action definition
Code = '2'  
Optional arguments: &lt;index&gt;,&lt;angle&gt;,&lt;delay&gt;  
index - position in the sequence to check [0, 15]  
angle - angle of servo to set in that action [0, 180]  
delay - delay after servo position is set in milliseconds [0, 60000]

## Debug port
Board sends various debug info in runtime over serial connection on pins 3 (TX), 2(RX) with baud rate of 115200.  
This could be used to debug if host software is performing actions correctly.
