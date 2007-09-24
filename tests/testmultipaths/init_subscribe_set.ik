subscribe set (id = 1, provider = 1, receiver = 2, forward = yes);
WAIT FOR EVENT (ORIGIN = ALL, CONFIRMED = ALL, WAIT ON = 2);
subscribe set (id = 1, provider = 1, receiver = 3, forward = yes);
WAIT FOR EVENT (ORIGIN = ALL, CONFIRMED = ALL, WAIT ON = 3);

subscribe set (id = 2, provider = 1, receiver = 2, forward = yes);
WAIT FOR EVENT (ORIGIN = ALL, CONFIRMED = ALL, WAIT ON = 2);
subscribe set (id = 2, provider = 2, receiver = 3, forward = yes);
WAIT FOR EVENT (ORIGIN = ALL, CONFIRMED = ALL, WAIT ON = 3);
    