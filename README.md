# ISAproject


// https://stackoverflow.com/questions/36221038/how-to-debug-a-forked-child-process-using-clion
When debugging forked process in CLion:
1. Set a break point at the beginning of your program (ie. the parent program, not the child program).
2. Start the program in the debugger.
3. Go to the debugger console (tab with the label gdb) in clion and enter set follow-fork-mode child and set detach-on-fork off.
4. Continue debugging.

set follow-fork-mode child
set detach-on-fork off