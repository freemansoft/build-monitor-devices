#!/usr/bin/perl
# Version 11.04 (c) Joe Freeman
# You can use this but give credit where it is due.
#
# This perl script talks with seedstudio cube over a USB-->TTL adapter based serial port.
# The Rainbowduino has a 5V (TTL) serial port on the connector on the back as documented
# You can create a cable for this and attach to a PC over a USB-->TTL adapter.
# Most hacked up phone cables are 3.3v which may get fried by 5V.
#
# Linux installation
# 	Install required perl serial port library (Device::SerialPort)
#		apt-get install libdevice-serialport-perl
# Windows
# 	Install strawberry perl http://strawberryperl.com/
# 	Install the windows serial port library
#		>perl -MCPAN -e shell
#		>install Win32::SerialPort
#		>exit
#		>perl nomake_install

#set up serial port
# couldn't get this to work in strawberry perl so you'll have to handle commenting/uncommenting
#if ($^O eq "MSWin32"){
	# -->> windows com port
	use Win32::SerialPort;
	my $port = new Win32::SerialPort("COM15");
#} else {
	# --> linux com port
	#use Device::SerialPort;
	#my $port = Device::SerialPort->new("/dev/ttyUSB0");
#}

$SET_ALL_CMD = 2;
$DRAW_DOT_CMD = 5;

$port->databits(8);
$port->baudrate(38400);           # 9600 default UartsB 38400 default BluetoothBee
$port->parity(none);
$port->stopbits(1);
$port->handshake("rts");
$port->read_char_time(000);      #don't wait for each character
$port->read_const_time(100);     #wait one tenth second before returning

$DEBUG = 0;

# R5pRGBd
# p=xyz combined
# RGB are 4 bit color
# d is don't care
sub sendPoint
{
	my $x = $_[0];
	my $y = $_[1];
	my $z = $_[2];
	my $coord = ($x << 4) | ($y <<2) | $z;
	my $r = $_[3];
	my $g = $_[4];
	my $b = $_[5];
	my $stringToSend = sprintf("R%c%c%c%c%c%c",$DRAW_DOT_CMD,$coord,$r,$g,$b,0);
	if ($DEBUG){
		my $commandHex = sprintf("R%d-%x-%x-%x-%x-%x",$DRAW_DOT_CMD,$coord,$r,$g,$b,0);
		my $debugString = sprintf "Command is %s - %i characters\n",$commandHex,length($stringToSend);
		print($debugString);
	}
	$port->write("$stringToSend");
}

sub setAll
{
	my $r = $_[0];
	my $g = $_[1];
	my $b = $_[2];
	my $stringToSend = sprintf("R%cx%c%c%cx",$SET_ALL_CMD,$r,$g,$b);
	if ($DEBUG){
		my $commandHex = sprintf("R%dx-%x-%x-%x-x",$SET_ALL_CMD,$r,$g,$b);
		my $debugString = sprintf "Command is %s - %i characters\n",$commandHex,length($stringToSend);
		print($debugString);
	}
	$port->write("$stringToSend");
}

sub clearAll
{
	setAll(0,0,0);
}

sub printReplyBuffer
{
	# only 255 is reliable according to docs
	my ($commandReplyCount,$commandReplyBuffer)=$port->read(255); 
	if ($commandReplyCount > 0){
		print "received:$commandReplyBuffer";
	}
}

# opening the connection causes a reset so we have to wait to restart
sleep(4);
# pick up the initialize message
printReplyBuffer();
# set everything to a pretty blue
setAll(0,0,9);
# let the slow humans see it
sleep(1);
# clear everything 
clearAll();
# let the humans see that it happened
sleep(1);

for ($ix = 0; $ix <= 3; $ix++) {
	for ($red = 0; $red <= 15; $red+=5){
		for ($green  = 0 ; $green <= 15; $green+=5){
			$blue = 15-$green;
			for ($iy = 0; $iy <=3; $iy++){
				for ($iz = 0; $iz <= 3; $iz++){
					if ($ix %2 == 0){
						sendPoint($ix,$iy,$iz,0,$green,$blue);
					} else {
						sendPoint($ix,$iy,$iz,$red,0,$blue);
					}
				}
			}
		}
	}
}

clearAll();

# loop through all the colors and all the lights
# do each slice in the x plane
for ($ix = 0; $ix <= 3; $ix++) {
	for ($red = 0; $red <= 15; $red+=3){
		for ($iz = 0; $iz <= 3; $iz++){
			for ($green = 0; $green <= 14; $green+=3){
				for ($iy = 0; $iy <=3; $iy++){
					for ($blue = 0; $blue <= 15; $blue+=3){
						if ($ix %2 == 0){
							sendPoint($ix,$iy,$iz,$red,$green,15-($red+$green));
						} else {
							sendPoint($ix,$iy,$iz,$red,15-($red+$blue),$blue);
						}
					}
				}
			}
		}
	}
}

sleep(8);
clearAll();

# linux is ok without this but windows complains
# the cube will reset on serial port close
undef $port;
