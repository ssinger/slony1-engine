	subscribe set ( id = 1, provider = 1, receiver = 2, forward = yes);
wait for event (origin = 1, confirmed = 2);
	subscribe set ( id = 1, provider = 2, receiver = 3, forward = no);
wait for event (origin = 1, confirmed = 3);
