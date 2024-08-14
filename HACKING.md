
Patches welcome!

## Structure

There are three classes of files:
 - ax25/: These make up the majority of the code providing AX.25.
   Due to this targetting embedded devices these cannot use almost anything from
   the standard library, but instead rely on platform.h.  The exception is
   memcpy and memcmp.

 - platform/: These provide the interface to the hardware.  There are currently
   two platforms, posix and null.

 - app/: These work with the platform files to provide a useful applications.

## Standard file contents
Each file should start with a comment with copyright information (including SPDX
tag) and at least a one line description of why this file exists.
Each .c file has a .h file that it's implementing.  Each .h file is self
contained and should include anything it requires.  To enforce this, .c files
include their respective .h file first, then followed by any other .h files
from the project (in asciibetical order), followed by system headers (again in
asciibetical order).

