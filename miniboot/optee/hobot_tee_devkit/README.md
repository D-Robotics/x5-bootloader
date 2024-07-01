# OP-TEE sanity testsuite
This git contains source code for the test suite (xtest) used to test the
OP-TEE project.

All official OP-TEE documentation has moved to http://optee.readthedocs.io. The
information that used to be here in this git can be found under [optee_test].

// OP-TEE core maintainers

[optee_test]: https://optee.readthedocs.io/en/latest/building/gits/optee_test.html

# TA Build Instructions

To build your Trusted Application (TA), follow these steps:

1. Place your TA source code in the `ta/customer` directory.

2. Open the `Makefile` located in the `ta/customer` directory.

3. Update the `CTA_DIRS` variable in the `Makefile` by adding your TA directory name.

4. Execute the following command to compile your TA:
   bash ./build.sh all

5. Once the build is complete, find your compiled TA files in the out/ta directory.

6. If you need to clean the build artifacts and start fresh, you can use the following command:
   bash ./build.sh clean
