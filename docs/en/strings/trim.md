# trim

* std::string &trim_right (std::string *str)
        
        Trims the given string in-place only on the left. Removes all characters in the 
        patterned way drop array, which defaults to ascii::is_space, return the reference 
        of the input string.
        
* std::string &trim_right (std::string *str, std::string_view drop)
    
        Trims the given string in-place only on the right. Removes all characters in
        the given drop array. return the reference of the input string.
    
* std::string_view trim_right (std::string_view str, std::string_view drop)
    
        Trims the given string only on the right. Removes all characters in the
        given drop array. Returns a copy of the string.
        
* std::string_view trim_right (std::string_view str)   
    
        Trims the given string only on the right. Removes all characters in the
        given drop array. Returns a copy of the string.
