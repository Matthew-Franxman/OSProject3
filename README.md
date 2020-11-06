# OSProject3
Thomas Cheal 5233790
Matthew Franxman 5070929

We attempted to fully implement the program, all of the required features are in there, however it does not quite work as intended. We use file locking, multithreading, a bounded buffer, and shared memory. The program is able to successfully search through all of the files given an input file, mostly finding the correct lines that correspond to the keywords. The program seems to run a lot better when manually tested, as it gives a different but more correct output as opposed to when run through the blackbox test. It can be demonstrated that manual testing worked better than the black box testing for our files.

The shared memory region is a array of strings that is sized according to the maximum length of the possible data stored with a space in between. The data stored is the directory that needs to be searched through and the keyword that is being searched for. An array was used as it was easier to work with inside the given shared memory space.
