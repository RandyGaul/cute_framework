# UTF8 and UTF16

Cute implements both utf8 and utf16 functions. There is a great website called http://utf8everywhere.org/ that contains a lot of great info on why you should simlpy use the utf8 encoding for all your strings everywhere. The gist is that utf8 is quite superior to all other encodings and should be used for pretty much everything all the time.

## Some advantages of UTF8

* Can encode anything utf32 can encode
* Is very widely used on the web and everywhere else
* Completely backwards compatible with typical ASCII
* Operations in utf8 buffers can be coded naively as if it were a regular old ASCII buffer
* utf8 buffers can always be treated as a naive byte buffer making the utf8 encoding scheme safe as an opaque data type
* Endianness independent
* Lexicographic sorting is identical to utf32 sorting

Unfortunately when dealing with Windows functions many APIs accept utf16 encodings in the form of wchar_t. As a response Cute has some functions for on-the-fly conversions to use utf16 called [widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md) and [shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md).

## Credit

The utf8 encoder/decoder was written by Richard Mitton for his lovely tigr library. A big thanks goes to Mitton for kindly releasing his code to public domain!

[decode8](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/decode8.md)  
[encode8](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/encode8.md)  
[codepoint8_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/codepoint8_size.md)  
[decode16](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/decode16.md)  
[encode16](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/encode16.md)  
[codepoint16_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/codepoint16_size.md)  
[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
[shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md)  
