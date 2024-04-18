changelog
====

# 0.9.88
1.update carbin cmake
2. remove log, EA using collie-log
3. remove flags, EA using collie-cli
4. fix gtest find case


# v0.7.0
1. simd unicode


# v6.16 plan

1. fmt cover the basic format function
2. Str* operations support both inlined_string and std::string as return and parameter 
eg. std::string r= StrFormat("{}", args); and inlined_string r = StrFormat("{}", args)