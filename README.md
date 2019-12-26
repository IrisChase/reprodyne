# Reprodyne
Reprodyne is a test harness component with the purpose transforming non-deterministic functions into deterministic functions, and to save serialized output from them for testing.

Reprodyne allows you to "intercept" indeterminate values (System events, network packets, time values, etc), serialize function calls utilizing this data, and then save to "tape". In playback mode, the indeterminates are then fed back into the functions in the order that they were originally created, and the serialized calls are then compared to the saved ones to ensure that the functions are behaving identically.

The operative idea is that, at least generally, real world data is both easier and faster to generate and save, and more useful to test against, than mocks.

Reprodyne was originally designed for use in end-to-end testing of GUI frameworks.


