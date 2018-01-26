This repository contains a ndnSIM scenario to evaluate bitvectors as a tool for
Data loop prevention when using Persistent Interests in NDN.

The scenario is build on the [ndnSIM scenario-template](https://github.com/named-data-ndnSIM/scenario-template). Detailed instructions for configuring scenarios can be also found in the referred repository.

## Cloning, and patching ndnSIM and running the scenario

As the implementation of Persistent Interests requires a modified version of
the NFD, the NFD has to be patched first. Files for patching NFD can be found
in the 'extern' folder.

Custom version of NS-3 and specified version of ndnSIM needs to be installed.

The code should also work with the latest version of ndnSIM, but it is not guaranteed.

    # Instructions for cloning repository
