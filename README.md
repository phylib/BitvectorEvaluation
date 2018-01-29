This repository contains a ndnSIM scenario to evaluate bitvectors as a tool for
Data loop prevention when using Persistent Interests in NDN.

The scenario is build on the [ndnSIM scenario-template](https://github.com/named-data-ndnSIM/scenario-template). Detailed instructions for configuring scenarios can be also found in the referred repository.

## Cloning, and patching ndnSIM and running the scenario

As the implementation of Persistent Interests requires a modified version of
the NFD, the NFD has to be patched first. Files for patching NFD can be found
in the 'extern' folder.

Custom version of NS-3 and specified version of ndnSIM needs to be installed.

The code should also work with the latest version of ndnSIM, but it is not guaranteed.

    # Checkout latest version of ndnSIM
    mdkir persistent-interests
    cd persistent-interests
    git clone https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
    git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
    git clone --recursive https://github.com/named-data-ndnSIM/ndnSIM.git ns-3/src/ndnSIM

    # Set correct version for ndnSIM and compile it
    cd ns-3
    git checkout 2c66f4c
    cd src/ndnSIM/
    git checkout 0970340
    cd NFD/
    git checkout 38111cd
    cd ../ndn-cxx/
    git checkout 4692ba8
    cd ../../../
    ./waf configure -d optimized
    ./waf
    sudo ./waf install

    # Checkout scenario
    cd ..
    git clone https://github.com/phylib/BitvectorEvaluation.git BitvectorEvaluation
    cd BitvectorEvaluation

    # Patch files of ndnSIM
    cp extern/ndnSIM/ndn-producer.hpp ../ns-3/src/ndnSIM/apps/ndn-producer.hpp
    cp extern/ndnSIM/ndn-link-control-helper.cpp ../ns-3/src/ndnSIM/helper/ndn-link-control-helper.cpp
    cp extern/ndnSIM/ndn-link-control-helper.hpp ../ns-3/src/ndnSIM/helper/ndn-link-control-helper.hpp
    cp extern/NFD/fw/* ../ns-3/src/ndnSIM/NFD/daemon/fw/
    cp extern/NFD/mgmt/* ../ns-3/src/ndnSIM/NFD/daemon/mgmt/
    cp extern/NFD/table/* ../ns-3/src/ndnSIM/NFD/daemon/table/
    cp extern/ndn-cxx/data.* ../ns-3/src/ndnSIM/ndn-cxx/src/
    cp extern/ndn-cxx/interest.* ../ns-3/src/ndnSIM/ndn-cxx/src/
    cp extern/ndn-cxx/tag-host.hpp ../ns-3/src/ndnSIM/ndn-cxx/src/
    cp extern/ndn-cxx/qci.hpp ../ns-3/src/ndnSIM/ndn-cxx/src/encoding/
    cp extern/ndn-cxx/tlv.hpp ../ns-3/src/ndnSIM/ndn-cxx/src/encoding/
    cp extern/ndn-cxx/nack-header.* ../ns-3/src/ndnSIM/ndn-cxx/src/lp/
