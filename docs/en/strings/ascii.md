# ascii

class ascii has some static method for char traits operation.
It has been designed to reduce computing, but using constant variable.
the interfaces of ascii have the same semantic  with std::isxxx.

## character_properties

a enum of character properties

enum | value
:--- | :---
eNone | 0x0
eControl | 0x0001
eSpace | 0x0002
ePunct | 0x0004
eDigit | 0x0008
eHexDigit | 0x0010
eAlpha | 0x0020
eLower | 0x0040
eUpper | 0x0080
eGraph | 0x0100
ePrint | 0x0200

a character may have several properties with operator or . character 'a' has five  properties.
eHexDigit, eAlpha, eLower, eGraph, ePrint, when we test if 'a' have some property, just compute with logical and.

### character_properties interface

*  character_properties operator & (character_properties lhs, character_properties rhs)

* character_properties operator | (character_properties lhs, character_properties rhs)

* character_properties operator ~ (character_properties lhs)

* character_properties operator ^ (character_properties lhs, character_properties rhs)

* character_properties &operator &= (character_properties &lhs, character_properties rhs)

* character_properties &operator |= (character_properties &lhs, character_properties rhs)

* character_properties &operator ^= (character_properties &lhs, character_properties rhs)

    
# interfaces

all the interface is static member function of ascii class.

* character_properties properties (unsigned char ch) noexcept
        
        get all properties of th given character
        
        e.g 

        auto p = ascii::properties('a');
        
*  bool has_properties (unsigned char ch, character_properties properties) noexcept
        
        test the given ch if have the properties or not, return true if match all of the given properties. 
        
* bool has_some_properties (unsigned char ch, character_properties properties) noexcept

        test the given ch if have the properties or not, return true if match one of the given properties. 
        
*  bool is_graph (unsigned char ch) noexcept

        test character if printabel or not, not contain space.
        
* bool is_digit (unsigned char ch) noexcept
        
        test character if digit or not.

* bool is_white (unsigned char ch) noexcept

        test character if print as a blank or change line.
        
* bool is_blank (unsigned char ch) noexcept

       test character if print as a blank.

* bool is_ascii (unsigned char ch) noexcept
        
        test character if ascii, ch & 0x80 == 0.
    
* bool is_space (unsigned char ch) noexcept

* bool is_hex_digit (unsigned char ch) noexcept

        
* bool is_punct (unsigned char ch) noexcept

* bool is_print (unsigned char ch) noexcept

* bool is_alpha (unsigned char ch) noexcept

* bool is_control (unsigned char ch) noexcept

* bool is_alpha_numeric (unsigned char ch) noexcept

* bool is_lower (unsigned char ch) noexcept

* bool is_upper (unsigned char ch) noexcept

* char to_upper (unsigned char ch) noexcept

* char to_lower (unsigned char ch) noexcept