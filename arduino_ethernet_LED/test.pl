# cpanm Net::Bonjour not needed  because we know the name
use HTTP::Request::Common;
use LWP;
use Time::HiRes; qw(usleep);

$ua = LWP::UserAgent->new;
#the Bonjour name we gave it in the arduino code
$url = 'http://arduino_led_strip.local/';
$num_leds = 30;
# blank the lights using the "all led" command
$ua->post($url,
	[ r  => '0',
	 g => '0',
	 b => '0',
	 l => '-1'
	],
);	

# now turn all of them on white using the bulk set command
# post accepts array ref or hash ref so lets create a hash ref
print "bam!  do them all in a single call (4 times)\n";
my $post_hash  ;
for ($color=3;$color>=0;$color--) {
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
for ($color=3;$color>=0;$color--) {
	$actual_color=$color*64;
	$i = 0;
	while ($i < $num_leds) {
		$one = $i+0;
		$two = $i+1;
		$three = $i+2;
		print "doing $one - $actual_color\n";
		$ua->post($url,
			[ r  => $actual_color,
			 g => '0',
			 b => '0',
			 l => $one
			],
		);	
		print "doing $two - $actual_color\n";
		$ua->post($url,
			[ r  => '0',
			 g => $actual_color,
			 b => '0',
			 l => $two
			],
		);	
		print "doing $three - $actual_color\n";
		$ua->post($url,
			[ r  => '0',
			 g => '0',
			 b => $actual_color,
			 l => $three
			],
		);	
		$i=$i+3;
	};
}

