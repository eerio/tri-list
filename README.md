# tri-list

This repository contains an implementation of a lazy C++ container, developed by me as part of a C++ programming course at the University of Warsaw (MIMUW).
The project was supposed to test our functional template programming in C++. It uses C++ `concept`s heavily. It also features a custom implementation of a C++ iterator, 
information about which are not particularly easy to find online (i.e. what methods do a class have to implement to be an iterator). It can be used in C++ `ranges` and features a lazy function application to the container`s elements (see the last snippet below)

Please for example take a look at this implementation of function composition:
https://github.com/eerio/tri-list/blob/6a6adb1456344048dba28c70dbd398651813a51c/tri_list.h#L127-L134

Or at this implementation of element access (with lazy modifiers application) which is a part of the iterator interface required by the C++ standard:
https://github.com/eerio/tri-list/blob/6a6adb1456344048dba28c70dbd398651813a51c/tri_list.h#L80-L85

It was fun! I learned much about templates in C++ (and about myself while debugging ;))
