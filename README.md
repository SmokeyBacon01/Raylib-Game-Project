# funny-game
A test game made with raylib to get use to game development

Hello yes, this project works now.
Run the executable with any command line argument to open in window mode (to view console).

! I downloaded raylib from the github repo and am linking the files directly in my Makefile, so the Makefile probably wont work for you !
! The raylib files aren't actually in this repo !

KNOWN ISSUES:
All good, I think...
Tesla enemies are still mildly unstable so keep an eye out for potential bugs.

As this project nears its end, keep these things in mind for the future;
- Write down and maintain a pipeline, many bugs were caused due to update order.
- Note down what functions are unsafe and where they fail. Leave asserts to help bugfix.
- More files. enemy.c grew to be too bloated.
- Don't overengineer solutions. Chasers took longer to develop than they had any right to. Make it work, then make it good.
