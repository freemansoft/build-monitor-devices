# cpanm Net::Bonjour not needed  because we know the name
use HTTP::Request::Common;
use LWP;
use Time::HiRes; qw(usleep);

$ua = LWP::UserAgent->new;
#the Bonjour name we gave it in the arduino code
$url = 'http://arduino_led.local/';
$num_leds = 30;
# blank the lights using the "all led" command
$ua->post($url,
	[ r  => '0',
	 g => '0',
	 b => '0',
	 l => '-1',
	 c => 'NA'
	],
);	

# now turn all of them on white using the bulk set command
# post accepts array ref or hash ref so lets create a hash ref
print "bam!  do them all in a single call (4 times)\n";
my $post_hash  ;
for ($color=3;$color>=0;$color--) {
	$post_hash{"s5"} = "Updating All";
	$actual_color=$color*64;
	for ($count = 0 ; $count < $num_leds; $count++){
		$post_hash{"r$count"} = $actual_color;
		$post_hash{"g$count"} = $actual_color;
		$post_hash{"b$count"} = $actual_color;
	}
	$ua->post($url,\%post_hash);
}

# turn on the lights one per time using the original single led command
# this is a LOT slower
# examples all use 
#		$ua->post($url,
#			[ r  => '0',
#			 g => '0',
#			 b => $actual_color,
#			 l => $three
#			],
#		);	

# demo one led per post
for ($color=3;$color>=0;$color--) {
	$actual_color=$color*64;
	$i = 0;
	while ($i < $num_leds) {
		$one = $i+0;
		$two = $i+1;
		$three = $i+2;
		print "doing $one - $actual_color\n";
		%{$post_hash} = ();
		$post_hash{"s5"} = "Updating $one";
		$post_hash{"r$one"} = $actual_color;
		$post_hash{"g$one"} = '0';
		$post_hash{"b$one"} = '0';
		$ua->post($url,\%post_hash);
		print "doing $two - $actual_color\n";
		%{$post_hash} = ();
		$post_hash{"s5"} = "Updating $two";
		$post_hash{"r$two"} = '0';
		$post_hash{"g$two"} =  $actual_color;
		$post_hash{"b$two"} = '0';
		$ua->post($url,\%post_hash);
		print "doing $three - $actual_color\n";
		%{$post_hash} = ();
		$post_hash{"s5"} = "Updating $three";
		$post_hash{"r$three"} = '0';
		$post_hash{"g$three"} = '0';
		$post_hash{"b$three"} = $actual_color;
		$ua->post($url,\%post_hash);
		$i=$i+3;
	};
}

