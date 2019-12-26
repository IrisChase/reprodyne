#Reprodyne
Reprodyne is a test harness component with the purpose transforming non-deterministic functions into deterministic ones for testing.

In short, Reprodyne allows you to "intercept" indeterminate values (System events, network packets, time values, etc), serialize function calls utilizing this data, and then save. In playback mode, the indeterminates are then fed back into the functions in the order that they were originally created, and the serialized calls are then compared to the saved ones to ensure that the functions are behaving identically.

The idea being, that real world data is easier to generate and save, and more useful to test against, than mocks (At least for some cases).

Reprodyne was originally designed for use in testing GUI frameworks.
