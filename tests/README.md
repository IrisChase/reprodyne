#Maintainers notes
These tests are ugly, but should be effective.

In the future I would like to generalize them, so that any future interceptors or validators can be tested by adding a couple of lines of code.

Currently, there is duplication between the validation tests in tests.cpp and videotests.cpp. I want to make it generic but I don't have the time.

Also, as is stated in videotests.cpp, a lot of scope stuff isn't done with them in particular. Again, it shouldn't be a problem, but a more robut testing solution would allow for this easily.
