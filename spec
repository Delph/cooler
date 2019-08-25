8 cooler modules
1 temperature sensor (OneWire)



Two things need to happen.
You have 8 outputs controlling cooler modules.
And a temperature probe running off onewire protocol.
I need the modules to turn on at dry temperatures to try to maintain  
18c.
And there’s an alarm pin to go off if the temp goes over 24c.
The alarm needs to arm only when the initial temperature has dropped 
below 19c.
Or it would go off constantly when I first start the system up.
Each output turns on .25c hotter then the last starting 
at 18c ending at 20c. As in at 20 their all on anyway.

So. If the modules are off too long. Their in contact with the hot 
water block. Meaning they then transfer heat through them into the 
cold water block. Losing efficiency like a stone. My idea is. To 
purge them of heat if they have been off for over five minutes. By 
powering for ten secs.
