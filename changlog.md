changelog
====
# v0.7.0
1. simd unicode


# v6.16 plan

1. fmt cover the basic format function
2. Str* operations support both inlined_string and std::string as return and parameter 
eg. std::string r= StrFormat("{}", args); and inlined_string r = StrFormat("{}", args)