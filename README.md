# SwapNet
Computer Netwroking CSC564 Group Project.

This is a toy project completed in my graduate program that gave an easy way to overwrite default libc netwroking functions for testing purposes.  This project includes an example that replaces networking with stop and wait.

Both the client and server program that are attempting to communicate will have to be intercepted or else the won't be able to communicate.

The motivating idea behind the project was to provide an easy way to swap out networking implementations to allow for easy testing on how different networking implementations might improve performance.

# how to use
Linux: ./intercept.sh ./myProgram

Mac: ./intercep_mac.sh ./myProgram
